#   Copyright (C) 2012 Texas Instruments Incorporated
#
#   All rights reserved. Property of Texas Instruments Incorporated.
#   Restricted rights to use, duplicate or disclose this code are
#   granted through contract.
#
#   The program may not be used without the written permission
#   of Texas Instruments Incorporated or against the terms and conditions
#   stipulated in the agreement under which this program has been
#   supplied.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    IDSSWBHal.cpp \
    DSSWBHal.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libutils \
    libbinder \
    libhardware

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../hwc/

LOCAL_MODULE:= libdsswbhal
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

