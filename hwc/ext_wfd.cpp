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

#include <stdint.h>
#include <sys/types.h>
#include <binder/IServiceManager.h>

#include "ext_wfd.h"

namespace android {

extern "C" {

static  sp<IWFD> wfdProxy = NULL;

static  sp<IWFD> get_wfdProxy()
{
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    status_t err = NO_ERROR;
    int count = 0;

    do {
        binder = sm->getService(String16("wfd.server"));
        if (binder != 0) {
            break;
        }
        //LOGW("WFD server not started, waiting...");
        usleep(500000); // 0.5 s
        count++;
    } while(count < 3); //loop 3 times with an interval of 0.5 sec to check wfdserver process

    if (binder != NULL) {
        wfdProxy = interface_cast<IWFD>(binder);
    } else {
        wfdProxy = NULL;
        LOGW("WFD server not started");
    }

    return wfdProxy;
}

int get_wfd_descriptor(int *fd)
{
    if (wfdProxy == NULL) {
        wfdProxy = get_wfdProxy();
    }

    if (wfdProxy == NULL) {
        return -1;
    }

    Parcel reply;
    status_t err = wfdProxy->getWFDDescriptor(&reply);

    if (err == NO_ERROR) {
        *fd= dup(reply.readFileDescriptor());
    } else {
        return -1;
    }

    return NO_ERROR;
}

int setup_wfd(wfd_config *cfg)
{
    if (wfdProxy == NULL) {
        wfdProxy = get_wfdProxy();
    }

    if (wfdProxy == NULL) {
        return -1;
    }

    wfdProxy->setupWFD(cfg);
    return NO_ERROR;
}

int teardown_wfd()
{
    if (wfdProxy == NULL) {
        wfdProxy = get_wfdProxy();
    }

    if (wfdProxy == NULL) {
        return -1;
    }

    wfdProxy->teardownWFD();
    return NO_ERROR;
}

}
}

