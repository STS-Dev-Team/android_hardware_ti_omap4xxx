/* ====================================================================
*   Copyright (C) 2012 Texas Instruments Incorporated
*
*   All rights reserved. Property of Texas Instruments Incorporated.
*   Restricted rights to use, duplicate or disclose this code are
*   granted through contract.
*
*   The program may not be used without the written permission
*   of Texas Instruments Incorporated or against the terms and conditions
*   stipulated in the agreement under which this program has been
*   supplied.
* ==================================================================== */

// Proxy for DSSWB HAL implementation

//#define LOG_NDEBUG 0
#define LOG_TAG "DSSWBHal"
#include <utils/Log.h>

#include <sys/types.h>
#include <string.h>

#include <cutils/atomic.h>
#include <utils/misc.h>

#include <binder/IServiceManager.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <utils/Vector.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "DSSWBHal.h"

#define HAL_PIXEL_FORMAT_TI_NV12 0x100

namespace android {

static DSSWBHal* gDSSWBHal;

DSSWBHal::DSSWBHal()
{
    LOGV("DSSWBHal constructor");
    mWBHandle = 0;
    mGrallocModule = NULL;
}

status_t DSSWBHal::initialize()
{
    status_t err;

    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t**)&mGrallocModule);
    if (err) {
        LOGE("unable to open gralloc module %d", err);
        return err;
    }

    srand(time(NULL));

    return NO_ERROR;
}

DSSWBHal::~DSSWBHal()
{
    LOGV("DSSWBHal destructor");
}

void DSSWBHal::instantiate()
{
    LOGV("DSSWBHal::instantiate");

    status_t err = NO_ERROR;

    if (!gDSSWBHal) {
        gDSSWBHal = new DSSWBHal();

        status_t err = gDSSWBHal->initialize();
        if (!err) {
            defaultServiceManager()->addService(String16("hardware.dsswb"), gDSSWBHal);
        }
    }
}

buffer_handle_t DSSWBHal::processQueue()
{
    //LOGV("DSSWBHal::processQueue");
    AutoMutex lock(mLock);

    if (mQueueList.empty()) {
        return NULL;
    }

    // remove the top buffer from queued list
    List<int>::iterator it;
    it = mQueueList.begin();
    int bufIndex = *it;

    // erase this buffer from queued list and update status
    mQueueList.erase(it);
    mBufList.editItemAt(bufIndex).state = BufList::WRITEBACK;

    // add this buffer to writeback list and give to hwc for capture
    mWritebackList.push_back(bufIndex);

    LOGV("processqueue returns index %d handle %p\n", bufIndex, mBufList[bufIndex].handle);
    return mBufList[bufIndex].handle;
}

void DSSWBHal::captureStarted(buffer_handle_t handle)
{
    AutoMutex lock(mLock);
    for (List<int>::iterator it = mWritebackList.begin(); it != mWritebackList.end(); ++it) {
        if (mBufList[*it].handle == handle) {
            // move this buffer from writeback to dequeue list and signal dequeue
            mDequeueList.push_back(*it);
            mWritebackList.erase(it);
            mDequeueCondition.signal();
            break;
        }
    }
}

void DSSWBHal::getConfig(wb_capture_config_t *config)
{
    AutoMutex lock(mLock);

    getConfigLocked(config);
}

bool DSSWBHal::capturePending()
{
    // TODO: can we capture display only if any layers are changing?
    return mWBHandle ? true : false;
//    return !mQueueList.empty();
}

status_t DSSWBHal::acquireWB(int *wbHandlePtr)
{
    LOGV("DSSWBHal::acquireWB");
    AutoMutex lock(mLock);
    if (mWBHandle != 0)
        return ALREADY_EXISTS;

    // assign dynamic value to make WB session secure
    mWBHandle = rand();
    *wbHandlePtr = mWBHandle;

    return NO_ERROR;
}

status_t DSSWBHal::releaseWB(int wbHandle)
{
    LOGV("DSSWBHal::releaseWB");
    AutoMutex lock(mLock);
    status_t err;

    if (wbHandle != mWBHandle)
        return PERMISSION_DENIED;

    // clear the queue and dequeue lists
    mQueueList.clear();
    mDequeueList.clear();

    // reset member variables at end of session
    for (Vector<BufList>::iterator it = mBufList.begin(); it != mBufList.end(); ++it) {
        err = mGrallocModule->unregisterBuffer(mGrallocModule, it->handle);
        if (err != 0)
            LOGW("unable to unregister buffer from SF allocator");
    }

    mBufList.clear();

    mWBHandle = 0;

    return NO_ERROR;
}

status_t DSSWBHal::registerBuffers(int wbHandle, int numBuffers, buffer_handle_t hList[])
{
    LOGV("DSSWBHal::registerBuffers");
    AutoMutex lock(mLock);
    status_t err;

    if (wbHandle != mWBHandle)
        return PERMISSION_DENIED;

    // allow buffer registration only once per WB session
    if (!mBufList.empty())
        return ALREADY_EXISTS;

    if (hList == NULL || numBuffers <= 0)
        return BAD_VALUE;

    for (int i = 0; i < numBuffers; ++i) {
        BufList buf;
        buf.handle = (native_handle_t *)hList[i];
        buf.state = BufList::DEQUEUED;

        err = mGrallocModule->registerBuffer(mGrallocModule, buf.handle);
        if (err) {
            LOGE("unable to register handle with SF allocator");
            return err;
        }

        mBufList.add(buf);
        LOGV("registered handle %p", buf.handle);
    }

    return NO_ERROR;
}

status_t DSSWBHal::queue(int wbHandle, int bufIndex)
{
    AutoMutex lock(mLock);
    //LOGV("DSSWBHal::queue");

    if (wbHandle != mWBHandle)
        return PERMISSION_DENIED;

    if (bufIndex < 0 || bufIndex >= (int)mBufList.size() || !mBufList[bufIndex].handle)
        return BAD_INDEX;

    if (mBufList[bufIndex].state == BufList::QUEUED)
        return ALREADY_EXISTS;

    if (mBufList[bufIndex].state == BufList::WRITEBACK)
        return INVALID_OPERATION;

    mQueueList.push_back(bufIndex);
    mBufList.editItemAt(bufIndex).state = BufList::QUEUED;

    LOGV("WBHal::queue index %d numqueued %d", bufIndex, mQueueList.size());
    return NO_ERROR;
}

status_t DSSWBHal::dequeue(int wbHandle, int *bufIndex)
{
    //LOGV("DSSWBHal::dequeue");
    AutoMutex lock(mLock);
    if (wbHandle != mWBHandle)
        return PERMISSION_DENIED;

    while ((!mQueueList.empty() || !mWritebackList.empty()) && mDequeueList.empty()) {
        LOGV("no buffers to dequeue numqueued %d", mQueueList.size());
        // wait for the queue to get one more buffer
        mDequeueCondition.wait(mLock);
    }

    if (mDequeueList.empty()) {
        return INVALID_OPERATION;
    }

    List<int>::iterator it;
    it = mDequeueList.begin();
    *bufIndex = *it;

    mDequeueList.erase(it);
    mBufList.editItemAt(*bufIndex).state = BufList::DEQUEUED;
    LOGV("WBHal::dequeue index %d status %d", *bufIndex, BufList::DEQUEUED);

    return NO_ERROR;
}

status_t DSSWBHal::cancelBuffer(int wbHandle, int *bufIndex)
{
    AutoMutex lock(mLock);
    LOGV("DSSWBHal::cancelBuffer");
    if (wbHandle != mWBHandle)
        return PERMISSION_DENIED;

    if (mQueueList.empty()) {
        LOGV("no buffers to cancel %d", mQueueList.size());
        // wait for the queue to get one more buffer
        return INVALID_OPERATION;
    }

    List<int>::iterator it;
    it = mQueueList.begin();
    *bufIndex = *it;

    mQueueList.erase(it);
    mBufList.editItemAt(*bufIndex).state = BufList::DEQUEUED;
    LOGV("WBHal::cancelBuffer index %d status %d", *bufIndex, BufList::DEQUEUED);
    mDequeueCondition.signal();
    return NO_ERROR;
}

status_t DSSWBHal::setConfig(int wbHandle, const wb_capture_config_t &config)
{
    // A limitation of decoupling config from buffer is that a config
    // is loosely associated with buffer and not tied too hard.
    LOGV("DSSWBHal::setConfig");
    AutoMutex lock(mLock);
    if (wbHandle != mWBHandle)
        return PERMISSION_DENIED;

    // TODO: need to check for capabilities before accepting the config
    mConfig = config;

    LOGV("Config transform %d", mConfig.transform);

    return NO_ERROR;
}

status_t DSSWBHal::getConfig(int wbHandle, wb_capture_config_t *config)
{
    LOGV("DSSWBHal::getConfig");
    AutoMutex lock(mLock);
    if (wbHandle != mWBHandle)
        return PERMISSION_DENIED;

    getConfigLocked(config);

    return NO_ERROR;
}

void DSSWBHal::getConfigLocked(wb_capture_config_t *config)
{
    *config = mConfig;
}

void wb_open()
{
    DSSWBHal::instantiate();
}

int wb_capture_layer(hwc_layer_t *wb_layer)
{
    wb_capture_config config;
    buffer_handle_t handle = gDSSWBHal->processQueue();

    // check if we have anything to capture
    if (handle == NULL)
        return 0;

    gDSSWBHal->getConfig(&config);

    //format the capture frame info as a layer
    wb_layer->handle = handle;

    wb_layer->transform = config.transform;
    wb_layer->displayFrame = config.captureFrame;
    wb_layer->sourceCrop = config.sourceCrop;

    // constant settings for WB layer, may use/change these later
#ifdef OMAP_ENHANCEMENT
    wb_layer->buf_layout = HWC_BUFFERS_LAYOUT_PROGRESSIVE;
#endif
    wb_layer->blending = HWC_BLENDING_NONE;
    wb_layer->compositionType = HWC_OVERLAY;
    wb_layer->hints = 0;
    wb_layer->flags = 0;

    return 1;
}

void wb_capture_started(buffer_handle_t handle)
{
    gDSSWBHal->captureStarted(handle);
}

int wb_capture_pending()
{
    return gDSSWBHal->capturePending();
}

};
