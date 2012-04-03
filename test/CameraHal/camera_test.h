#ifndef CAMERA_TEST_H
#define CAMERA_TEST_H

#define PRINTOVER(arg...)     LOGD(#arg)
#define LOG_FUNCTION_NAME         LOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define LOG_FUNCTION_NAME_EXIT    LOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);
#define KEY_GBCE            "gbce"
#define KEY_GLBCE           "glbce"
#define KEY_CAMERA          "camera-index"
#define KEY_SATURATION      "saturation"
#define KEY_BRIGHTNESS      "brightness"
#define KEY_TI_BURST           "burst-capture"
#define KEY_EXPOSURE        "exposure"
#define KEY_CONTRAST        "contrast"
#define KEY_SHARPNESS       "sharpness"
#define KEY_ISO             "iso"
#define KEY_CAF             "caf"
#define KEY_MODE            "mode"
#define KEY_VNF             "vnf"
#define KEY_VSTAB           "vstab"
#define KEY_COMPENSATION    "exposure-compensation"
#define KEY_SENSOR_ORIENTATION "sensor-orientation"

#define KEY_IPP             "ipp"

#define KEY_BUFF_STARV      "buff-starvation"
#define KEY_METERING_MODE   "meter-mode"
#define KEY_AUTOCONVERGENCE "auto-convergence-mode"
#define KEY_MANUAL_CONVERGENCE "manual-convergence"
#define KEY_EXP_BRACKETING_RANGE "exp-bracketing-range"
#define KEY_TEMP_BRACKETING "temporal-bracketing"
#define KEY_TEMP_BRACKETING_POS "temporal-bracketing-range-positive"
#define KEY_TEMP_BRACKETING_NEG "temporal-bracketing-range-negative"
#define KEY_MEASUREMENT "measurement"
#define KEY_S3D2D_PREVIEW_MODE "s3d2d-preview"
#define KEY_STEREO_CAMERA "s3d-supported"
#define KEY_S3D_PRV_FRAME_LAYOUT "s3d-prv-frame-layout"
#define KEY_S3D_CAP_FRAME_LAYOUT "s3d-cap-frame-layout"
#define KEY_EXIF_MODEL "exif-model"
#define KEY_EXIF_MAKE "exif-make"
#define KEY_AF_TIMEOUT "af-timeout"

#define KEY_AUTO_EXPOSURE_LOCK "auto-exposure-lock"
#define KEY_AUTO_WHITEBALANCE_LOCK "auto-whitebalance-lock"

#define KEY_MECHANICAL_MISALIGNMENT_CORRECTION "mechanical-misalignment-correction"

//TI extensions for enable/disable algos
#define KEY_ALGO_FIXED_GAMMA            "ti-algo-fixed-gamma"
#define KEY_ALGO_NSF1                   "ti-algo-nsf1"
#define KEY_ALGO_NSF2                   "ti-algo-nsf2"
#define KEY_ALGO_SHARPENING             "ti-algo-sharpening"
#define KEY_ALGO_THREELINCOLORMAP       "ti-algo-threelinecolormap"
#define KEY_ALGO_GIC                    "ti-algo-gic"

#define SDCARD_PATH "/sdcard/"

#define MAX_BURST   15
#define BURST_INC     5
#define TEMP_BRACKETING_MAX_RANGE 4

#define MEDIASERVER_DUMP "procmem -w $(ps | grep mediaserver | grep -Eo '[0-9]+' | head -n 1) | grep \"\\(Name\\|libcamera.so\\|libOMX\\|libomxcameraadapter.so\\|librcm.so\\|libnotify.so\\|libipcutils.so\\|libipc.so\\|libsysmgr.so\\|TOTAL\\)\""
#define MEMORY_DUMP "procrank -u"
#define KEY_METERING_MODE   "meter-mode"

#define TEST_FOCUS_AREA "(-1000,500,-500,1000,1000),(500,500,1000,1000,1)"
#define TEST_METERING_AREA "(-1000,500,-500,1000,1000),(500,500,1000,1000,1)"
#define TEST_METERING_AREA_CENTER "(-250,-250,250,250,1000)"
#define TEST_METERING_AREA_AVERAGE "(-1000,-1000,1000,1000,1000)"

#define COMPENSATION_OFFSET 20
#define DELIMITER           "|"

#define MODEL "camera_test"
#define MAKE "camera_test"

#define BLAZE 0
#define BLAZE_TABLET1 1
#define BLAZE_TABLET2 2

#define MAX_LINES    80
#define MAX_SYMBOLS  65

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef enum test_type {
    TEST_TYPE_REGRESSION,
    TEST_TYPE_FUNCTIONAL,
    TEST_TYPE_API,
    TEST_TYPE_ERROR,
} test_type_t;

enum logging {
    LOGGING_LOGCAT = 0x01,
    LOGGING_SYSLINK = 0x02
};

typedef struct cmd_args {
    test_type_t test_type;
    const char *script_file_name;
    const char *output_path;
    int platform_id;
    int logging;
} cmd_args_t;

namespace android {
    class CameraHandler: public CameraListener {
        public:
            virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
            virtual void postData(int32_t msgType,
                                  const sp<IMemory>& dataPtr,
                                  camera_frame_metadata_t *metadata);

            virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);
    };

};

using namespace android;


typedef struct pixel_format_t {
    int32_t pixelFormatDesc;
    const char *pixformat;
}pixel_format;

typedef struct output_format_t {
    output_format type;
    const char *desc;
} outformat;

typedef struct Vcapture_size_t {
    int width, height;
    const char *desc;
} Vcapture_size;

typedef struct param_Array_t {
    int width, height;
    char name[60];
} param_Array;

typedef struct video_Codecs_t {
    video_encoder type;
    const char *desc;
} video_Codecs;

typedef struct audio_Codecs_t {
    audio_encoder type;
    const char *desc;
} audio_Codecs;

typedef struct V_bitRate_t {
    uint32_t bit_rate;
    const char *desc;
} V_bitRate;

typedef struct zoom_t {
    int idx;
    const char *zoom_description;
} Zoom;

typedef struct fps_ranges_t {
    int rangeMin;
    int rangeMax;
} fps_Array;

char * get_cycle_cmd(const char *aSrc);
int execute_functional_script(char *script);
status_t dump_mem_status();
int openCamera();
int closeCamera();
void initDefaults();
int startPreview();
void stopPreview();
int startRecording();
int stopRecording();
int closeRecorder();
int openRecorder();
int configureRecorder();
void printSupportedParams();
char *load_script(const char *config);
int start_logging(int flags, int &pid);
int stop_logging(int flags, int &pid);
int execute_error_script(char *script);
int getParametersFromCapabilities();
void  getSizeParametersFromCapabilities();
int getDefaultParameter(const char* val, int numOptions, char **array);
int getDefaultParameterResol(const char* val, int numOptions, param_Array  **array);
int getSupportedParameters(char* parameters, int* optionsCount, char ***elem);
int getSupportedParametersCaptureSize(char* parameters, int *optionsCount, param_Array array[], int arraySize);
int getSupportedParametersVideoCaptureSize(char* parameters, int *optionsCount, param_Array array[], int arraySize);
int getSupportedParametersPreviewSize(char* parameters, int *optionsCount, param_Array array[], int arraySize);
int getSupportedParametersThumbnailSize(char* parameters, int *optionsCount, param_Array array[], int arraySize);
int getSupportedParametersNames(int width, int height, param_Array array[], int arraySize);
int checkSupportedParamScript(char **array, int size, char *param);
int checkSupportedParamScriptResol(param_Array **array, int size, char *param, int *num);
int getSupportedParametersfps(char* parameters, int *optionsCount);
int checkSupportedParamScriptfpsConst(int *array, int size, char *param, int *num);
int checkSupportedParamScriptfpsRange(char **array, int size, char *param, int *num);
int trySetVideoStabilization(bool toggle);
int trySetVideoNoiseFilter(bool toggle);
int trySetAutoExposureLock(bool toggle);
int trySetAutoWhiteBalanceLock(bool toggle);
int deleteAllocatedMemory();
void initDefaultsSec();

const char KEY_S3D_PRV_FRAME_LAYOUT_VALUES[] = "s3d-prv-frame-layout-values";
const char KEY_S3D_CAP_FRAME_LAYOUT_VALUES[] = "s3d-cap-frame-layout-values";
const char KEY_SUPPORTED_PICTURE_TOPBOTTOM_SIZES[] = "supported-picture-topbottom-size-values";
const char KEY_SUPPORTED_PICTURE_SIDEBYSIDE_SIZES[] = "supported-picture-sidebyside-size-values";
const char KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES[] = "supported-preview-topbottom-size-values";
const char KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES[] = "supported-preview-sidebyside-size-values";
const char KEY_AUTOCONVERGENCE_MODE[] = "auto-convergence-mode";
const char KEY_AUTOCONVERGENCE_MODE_VALUES[] = "auto-convergence-mode-values";
const char KEY_SUPPORTED_MANUAL_CONVERGENCE_MIN[] = "supported-manual-convergence-min";
const char KEY_SUPPORTED_MANUAL_CONVERGENCE_MAX[] = "supported-manual-convergence-max";
const char KEY_SUPPORTED_MANUAL_CONVERGENCE_STEP[] = "supported-manual-convergence-step";

#endif
