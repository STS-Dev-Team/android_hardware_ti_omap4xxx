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

#ifndef ANDROID_EXT_WFD_H
#define ANDROID_EXT_WFD_H

#include <IWFD.h>

#ifdef __cplusplus
extern "C" int get_wfd_descriptor(int *fd);
extern "C" int setup_wfd(wfd_config *wc);
extern "C" int teardown_wfd();

#else
extern int get_wfd_descriptor(int *fd);
extern int setup_wfd(wfd_config *wc);
extern int teardown_wfd();
#endif

#endif // ANDROID_EXT_WFD_H

