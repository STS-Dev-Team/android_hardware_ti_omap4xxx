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

#ifndef ANDROID_HARDWARE_IDSSWB_HAL_H
#define ANDROID_HARDWARE_IDSSWB_HAL_H

#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <hardware/gralloc.h>
#include <hardware/hwcomposer.h>

namespace android {

typedef struct wb_capture_config {
    /* handle of buffer to compose. this handle is guaranteed to have been
     * allocated with gralloc */
   // buffer_handle_t handle;

    /* transformation to apply to the buffer during composition */
    int transform;

    /* area of the source to consider, the origin is the top-left corner of
     * the screen */
    hwc_rect_t sourceCrop;

    /* where to capture the sourceCrop into the buffer. The sourceCrop
     * is scaled using linear filtering to the captureFrame. The origin is the
     * top-left corner of the buffer.
     */
    hwc_rect_t captureFrame;
} wb_capture_config_t;

class IDSSWBHal: public IInterface
{
public:
    DECLARE_META_INTERFACE(DSSWBHal);

    virtual status_t         acquireWB(int *wbHandlePtr) = 0;
    virtual status_t         releaseWB(int wbHandle) = 0;
    virtual status_t         registerBuffers(int wbHandle, int numBuffers, buffer_handle_t hList[]) = 0;
    virtual status_t         queue(int wbHandle, int bufIndex) = 0;
    virtual status_t         dequeue(int wbHandle, int *bufIndex) = 0;
    virtual status_t         cancelBuffer(int wbHandle, int *bufIndex) = 0;
    virtual status_t         setConfig(int wbHandle, const wb_capture_config_t &config) = 0;
    virtual status_t         getConfig(int wbHandle, wb_capture_config_t *config) = 0;
};

// ----------------------------------------------------------------------------

class BnDSSWBHal: public BnInterface<IDSSWBHal>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // ANDROID_HARDWARE_IDSSWB_HAL_H
