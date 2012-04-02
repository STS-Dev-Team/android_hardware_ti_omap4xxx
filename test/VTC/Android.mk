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
    $(DOMX_PATH)/omx_core/inc \

LOCAL_CFLAGS +=-Wall -fno-short-enums -O0 -g -D___ANDROID___

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := VTCTestApp
include $(BUILD_EXECUTABLE)

###############################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := VTCLoopback.cpp IOMXEncoder.cpp IOMXDecoder.cpp

LOCAL_SHARED_LIBRARIES := \
    libcamera_client \
    libstagefright \
    libmedia \
    libbinder \
    libtiutils \
    libcutils \
    libutils \
    liblog \
    libgui \
    libui \

LOCAL_C_INCLUDES += \
    $(TOP)/frameworks/base/include/ui \
    $(TOP)/frameworks/base/include/surfaceflinger \
    $(TOP)/frameworks/base/include/camera \
    $(TOP)/frameworks/base/include/media \
    $(TOP)/frameworks/base/include/media/stagefright \
    $(TOP)/hardware/ti/domx/omx_core/inc \
    $(TOP)/hardware/ti/omap4xxx/libtiutils

LOCAL_CFLAGS +=-Wall -fno-short-enums -O0 -g

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := VTCLoopbackTest
include $(BUILD_EXECUTABLE)

###############################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= CameraHardwareInterfaceTest.cpp

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libutils \
    libcutils \
    libbinder \
    libmedia \
    libui \
    libgui \
    libcamera_client \
    libhardware

LOCAL_C_INCLUDES += \
    frameworks/base/include/ui \
    frameworks/base/include/surfaceflinger \
    frameworks/base/include/camera \
    frameworks/base/include/media \
    frameworks/base/services/camera/libcameraservice \
    $(PV_INCLUDES)

LOCAL_MODULE:= CameraHardwareInterfaceTest
LOCAL_MODULE_TAGS:= tests

LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
    LOCAL_CFLAGS += -DTARGET_OMAP4
endif

include $(BUILD_HEAPTRACKED_EXECUTABLE)

