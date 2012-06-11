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

#ifndef ANDROID_HARDWARE_DSSWB_HAL_CLIENT_H
#define ANDROID_HARDWARE_DSSWB_HAL_CLIENT_H

#ifdef __cplusplus

#include <stdbool.h>

#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include <hardware/gralloc.h>
#include <IDSSWBHal.h>

namespace android {

class DSSWBHal : public BnDSSWBHal
{
private:
    DSSWBHal();

    status_t                          initialize();

public:
    virtual ~DSSWBHal();

    static void                       instantiate();
    buffer_handle_t                   processQueue();
    void                              captureStarted(buffer_handle_t handle);
    bool                              capturePending();
    void                              getConfig(wb_capture_config_t *config);
    // IDSSWBHal interface
    virtual status_t                  acquireWB(int *wbHandlePtr);
    virtual status_t                  releaseWB(int wbHandle);
    virtual status_t                  registerBuffers(int wbHandle, int numBuffers, buffer_handle_t hList[]);
    virtual status_t                  queue(int wbHandle, int bufIndex);
    virtual status_t                  dequeue(int wbHandle, int *bufIndex);
    virtual status_t                  cancelBuffer(int wbHandle, int *bufIndex);
    virtual status_t                  setConfig(int wbHandle, const wb_capture_config_t &config);
    virtual status_t                  getConfig(int wbHandle, wb_capture_config_t *config);

private:
    void                              getConfigLocked(wb_capture_config_t *config);

private:
    int                               mWBHandle;
    Mutex                             mLock;
    wb_capture_config_t               mConfig;

    struct BufList {
        native_handle_t *handle;
        enum BufState {
            QUEUED = 0,
            WRITEBACK = 1,
            DEQUEUED = 2,
        };
        BufState state;
    };

    Vector<BufList>                    mBufList;

    List<int>                          mQueueList;
    List<int>                          mWritebackList;
    List<int>                          mDequeueList;
    // mDequeueCondition condition used for dequeueBuffer in synchronous mode
    mutable Condition                  mDequeueCondition;
    gralloc_module_t                   *mGrallocModule;

// ----------------------------------------------------------------------------
};

/* the declaration of functions being used from hwc.c need to be
 * "C" or else the name gets mangled and hwc library will not build
 * against these functions. So two declarations of this function. One in
 * C++ which will create the library with right "C" name and other in
 * C which is for hwc.c when it includes this file.
 */
extern "C" void wb_open();
extern "C" int wb_capture_layer(hwc_layer_t *wb_layer);
extern "C" void wb_capture_started(buffer_handle_t handle);
extern "C" int wb_capture_pending();
};
#else
#include <hardware/hwcomposer.h>

extern void wb_open();
extern int wb_capture_layer(hwc_layer_t *wb_layer);
extern void wb_capture_started(buffer_handle_t handle);
extern int wb_capture_pending();
#endif // __cplusplus

#endif // ANDROID_HARDWARE_DSSWB_HAL_CLIENT_H
