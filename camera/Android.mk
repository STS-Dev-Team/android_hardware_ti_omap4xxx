ifeq ($(TARGET_BOARD_PLATFORM),omap4)

LOCAL_PATH:= $(call my-dir)

OMAP4_CAMERA_HAL_USES:= OMX
# OMAP4_CAMERA_HAL_USES:= USB

ifdef TI_CAMERAHAL_DEBUG_ENABLED
    # Enable CameraHAL debug logs
    CAMERAHAL_CFLAGS += -DCAMERAHAL_DEBUG
endif

ifdef TI_CAMERAHAL_VERBOSE_DEBUG_ENABLED
    # Enable CameraHAL verbose debug logs
    CAMERAHAL_CFLAGS += -DCAMERAHAL_DEBUG_VERBOSE
endif

ifdef TI_CAMERAHAL_DEBUG_FUNCTION_NAMES
    # Enable CameraHAL function enter/exit logging
    CAMERAHAL_CFLAGS += -DTI_UTILS_FUNCTION_LOGGER_ENABLE
endif

ifdef TI_CAMERAHAL_DEBUG_TIMESTAMPS
    # Enable timestamp logging
    CAMERAHAL_CFLAGS += -DTI_UTILS_DEBUG_USE_TIMESTAMPS
endif

ifndef TI_CAMERAHAL_DONT_USE_RAW_IMAGE_SAVING
    # Enabled saving RAW images to file
    CAMERAHAL_CFLAGS += -DCAMERAHAL_USE_RAW_IMAGE_SAVING
endif

CAMERAHAL_CFLAGS += -DLOG_TAG=\"CameraHal\"

OMAP4_CAMERA_HAL_SRC := \
	CameraHal_Module.cpp \
	CameraHal.cpp \
	CameraHalUtilClasses.cpp \
	AppCallbackNotifier.cpp \
	ANativeWindowDisplayAdapter.cpp \
	CameraProperties.cpp \
	MemoryManager.cpp \
	Encoder_libjpeg.cpp \
	SensorListener.cpp  \
	NV12_resize.c

OMAP4_CAMERA_COMMON_SRC:= \
	CameraParameters.cpp \
	TICameraParameters.cpp \
	CameraHalCommon.cpp

OMAP4_CAMERA_OMX_SRC:= \
	BaseCameraAdapter.cpp \
	OMXCameraAdapter/OMX3A.cpp \
	OMXCameraAdapter/OMXAlgo.cpp \
	OMXCameraAdapter/OMXCameraAdapter.cpp \
	OMXCameraAdapter/OMXCapabilities.cpp \
	OMXCameraAdapter/OMXCapture.cpp \
	OMXCameraAdapter/OMXDefaults.cpp \
	OMXCameraAdapter/OMXExif.cpp \
	OMXCameraAdapter/OMXFD.cpp \
	OMXCameraAdapter/OMXFocus.cpp \
	OMXCameraAdapter/OMXZoom.cpp \
	OMXCameraAdapter/OMXDccDataSave.cpp \

OMAP4_CAMERA_USB_SRC:= \
	BaseCameraAdapter.cpp \
	V4LCameraAdapter/V4LCameraAdapter.cpp

#
# OMX Camera HAL
#

ifeq ($(OMAP4_CAMERA_HAL_USES),OMX)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	$(OMAP4_CAMERA_HAL_SRC) \
	$(OMAP4_CAMERA_OMX_SRC) \
	$(OMAP4_CAMERA_COMMON_SRC)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/ \
    $(LOCAL_PATH)/../hwc \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/inc/OMXCameraAdapter \
    $(LOCAL_PATH)/../libtiutils \
    hardware/ti/omap4xxx/tiler \
    hardware/ti/omap4xxx/ion \
    frameworks/base/include/ui \
    frameworks/base/include/utils \
    $(DOMX_PATH)/omx_core/inc \
    $(DOMX_PATH)/mm_osal/inc \
    frameworks/base/include/media/stagefright \
    frameworks/base/include/media/stagefright/openmax \
    external/jpeg \
    external/jhead

LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libmm_osal \
    libOMX_Core \
    libcamera_client \
    libgui \
    libdomx \
    libion \
    libjpeg \
    libexif

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER $(CAMERAHAL_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

else
ifeq ($(OMAP4_CAMERA_HAL_USES),USB)

#
# USB Camera Adapter
#

include $(CLEAR_VARS)

CAMERAHAL_CFLAGS += -DV4L_CAMERA_ADAPTER

LOCAL_SRC_FILES:= \
	$(OMAP4_CAMERA_HAL_SRC) \
	$(OMAP4_CAMERA_USB_SRC) \
	$(OMAP4_CAMERA_COMMON_SRC)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/ \
    $(LOCAL_PATH)/../hwc \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/inc/V4LCameraAdapter \
    $(LOCAL_PATH)/../libtiutils \
    hardware/ti/omap4xxx/tiler \
    hardware/ti/omap4xxx/ion \
    frameworks/base/include/ui \
    frameworks/base/include/utils \
    frameworks/base/include/media/stagefright \
    frameworks/base/include/media/stagefright/openmax \
    external/jpeg \
    external/jhead

LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libcamera_client \
    libgui \
    libion \
    libjpeg \
    libexif

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER $(CAMERAHAL_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)
endif
endif
endif
