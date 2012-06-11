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

//#define LOG_NDEBUG 0
#define LOG_TAG "IDSSWBHal"
#include <utils/Log.h>

#include <stdint.h>

#include <binder/Parcel.h>
#include <utils/Errors.h>

#include "IDSSWBHal.h"

namespace android {

enum {
    ACQUIRE_WB = IBinder::FIRST_CALL_TRANSACTION,
    RELEASE_WB,
    REGISTER_BUFFERS,
    QUEUE,
    DEQUEUE,
    CANCEL_BUFFER,
    SET_CONFIG,
    GET_CONFIG
};

class BpDSSWBHal: public BpInterface<IDSSWBHal>
{
public:
    BpDSSWBHal(const sp<IBinder>& impl)
        : BpInterface<IDSSWBHal>(impl)
    {
    }

    virtual status_t acquireWB(int *wbHandlePtr)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        remote()->transact(ACQUIRE_WB, data, &reply);
        *wbHandlePtr = reply.readInt32();
        return reply.readInt32();
    }

    virtual status_t releaseWB(int wbHandle)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        data.writeInt32(wbHandle);
        remote()->transact(RELEASE_WB, data, &reply);
        return reply.readInt32();
    }

    virtual status_t registerBuffers(int wbHandle, int numBuffers, buffer_handle_t hList[])
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        data.writeInt32(wbHandle);
        LOGD("BpDSSWBHal::registerBuffers numBuffers %d ", numBuffers);

        data.writeInt32(numBuffers);

        for (int i = 0; i < numBuffers; ++i)
            data.writeNativeHandle(hList[i]);

        remote()->transact(REGISTER_BUFFERS, data, &reply);
        return reply.readInt32();
    }

    virtual status_t queue(int wbHandle, int bufIndex)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        data.writeInt32(wbHandle);
        data.writeInt32(bufIndex);
        remote()->transact(QUEUE, data, &reply);
        return reply.readInt32();
    }

    virtual status_t dequeue(int wbHandle, int *bufIndex)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        data.writeInt32(wbHandle);
        remote()->transact(DEQUEUE, data, &reply);
        *bufIndex = reply.readInt32();
        return reply.readInt32();
    }

    virtual status_t cancelBuffer(int wbHandle, int *bufIndex)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        data.writeInt32(wbHandle);
        remote()->transact(CANCEL_BUFFER, data, &reply);
        *bufIndex = reply.readInt32();
        return reply.readInt32();
    }

    virtual status_t setConfig(int wbHandle, const wb_capture_config_t &config)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        data.writeInt32(wbHandle);
        data.writeInt32(config.transform);

        data.writeInt32(config.sourceCrop.left);
        data.writeInt32(config.sourceCrop.top);
        data.writeInt32(config.sourceCrop.right);
        data.writeInt32(config.sourceCrop.bottom);

        data.writeInt32(config.captureFrame.left);
        data.writeInt32(config.captureFrame.top);
        data.writeInt32(config.captureFrame.right);
        data.writeInt32(config.captureFrame.bottom);

        remote()->transact(SET_CONFIG, data, &reply);
        return reply.readInt32();
    }

    virtual status_t getConfig(int wbHandle, wb_capture_config_t *config)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDSSWBHal::getInterfaceDescriptor());
        data.writeInt32(wbHandle);
        remote()->transact(GET_CONFIG, data, &reply);

        config->transform = reply.readInt32();

        config->sourceCrop.left = reply.readInt32();
        config->sourceCrop.top = reply.readInt32();
        config->sourceCrop.right = reply.readInt32();
        config->sourceCrop.bottom = reply.readInt32();

        config->captureFrame.left = reply.readInt32();
        config->captureFrame.top = reply.readInt32();
        config->captureFrame.right = reply.readInt32();
        config->captureFrame.bottom = reply.readInt32();

        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(DSSWBHal, "android.hardware.IDSSWBHal");

// ----------------------------------------------------------------------

status_t BnDSSWBHal::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case ACQUIRE_WB: {
            int wbHandle;
            status_t ret;
            CHECK_INTERFACE(IDSSWBHal, data, reply);
            ret = acquireWB(&wbHandle);
            reply->writeInt32(wbHandle);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case RELEASE_WB: {
            CHECK_INTERFACE(IDSSWBHal, data, reply);
            int wbHandle = data.readInt32();
            reply->writeInt32(releaseWB(wbHandle));
            return NO_ERROR;
        } break;
        case REGISTER_BUFFERS: {
            CHECK_INTERFACE(IDSSWBHal, data, reply);
            int wbHandle = data.readInt32();
            int numBuffers = data.readInt32();
            buffer_handle_t hList[numBuffers];

            for (int i = 0; i < numBuffers; ++i)
                hList[i] = data.readNativeHandle();

            reply->writeInt32(registerBuffers(wbHandle, numBuffers, hList));
            return NO_ERROR;
        } break;
        case QUEUE: {
            CHECK_INTERFACE(IDSSWBHal, data, reply);
            int wbHandle = data.readInt32();
            int bufIndex = data.readInt32();
            reply->writeInt32(queue(wbHandle, bufIndex));
            return NO_ERROR;
        } break;
        case DEQUEUE: {
            CHECK_INTERFACE(IDSSWBHal, data, reply);
            int wbHandle = data.readInt32();
            int bufIndex;
            status_t ret = dequeue(wbHandle, &bufIndex);
            reply->writeInt32(bufIndex);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case CANCEL_BUFFER: {
            CHECK_INTERFACE(IDSSWBHal, data, reply);
            int wbHandle = data.readInt32();
            int bufIndex;
            status_t ret = cancelBuffer(wbHandle, &bufIndex);
            reply->writeInt32(bufIndex);
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case SET_CONFIG: {
            wb_capture_config_t config;
            int wbHandle;
            CHECK_INTERFACE(IDSSWBHal, data, reply);
            wbHandle = data.readInt32();

            config.transform = data.readInt32();

            config.sourceCrop.left = data.readInt32();
            config.sourceCrop.top = data.readInt32();
            config.sourceCrop.right = data.readInt32();
            config.sourceCrop.bottom = data.readInt32();

            config.captureFrame.left = data.readInt32();
            config.captureFrame.top = data.readInt32();
            config.captureFrame.right = data.readInt32();
            config.captureFrame.bottom = data.readInt32();

            reply->writeInt32(setConfig(wbHandle, config));
            return NO_ERROR;
        } break;
        case GET_CONFIG: {
            wb_capture_config_t config;
            status_t ret;
            CHECK_INTERFACE(IDSSWBHal, data, reply);

            ret = getConfig(data.readInt32(), &config);
            reply->writeInt32(config.transform);

            reply->writeInt32(config.sourceCrop.left);
            reply->writeInt32(config.sourceCrop.top);
            reply->writeInt32(config.sourceCrop.right);
            reply->writeInt32(config.sourceCrop.bottom);

            reply->writeInt32(config.captureFrame.left);
            reply->writeInt32(config.captureFrame.top);
            reply->writeInt32(config.captureFrame.right);
            reply->writeInt32(config.captureFrame.bottom);

            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
