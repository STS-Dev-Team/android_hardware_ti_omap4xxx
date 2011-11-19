LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := VTCTestApp.cpp

LOCAL_SHARED_LIBRARIES := \
    libcamera_client \
    libstagefright \
    libmedia \
    libbinder \
    libcutils \
    libutils \
    liblog \
    libgui \

LOCAL_C_INCLUDES += \
    $(TOP)/frameworks/base/include/ui \
    $(TOP)/frameworks/base/include/surfaceflinger \
    $(TOP)/frameworks/base/include/camera \
    $(TOP)/frameworks/base/include/media \
    $(TOP)/frameworks/base/include/media/stagefright \
    $(TOP)/frameworks/base/include/media/stagefright/openmax \
    $(TOP)/hardware/ti/omap4xxx/domx/omx_core/inc \

LOCAL_CFLAGS +=-Wall -fno-short-enums -O0 -g -D___ANDROID___

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := VTCTestApp
include $(BUILD_EXECUTABLE)

