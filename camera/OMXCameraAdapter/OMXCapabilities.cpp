/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file OMXCap.cpp
*
* This file implements the OMX Capabilities feature.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"
#include "TICameraParameters.h"

namespace android {

/************************************
 * global constants and variables
 *************************************/

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))
#define FPS_MIN 5
#define FPS_STEP 5
#define FPS_RANGE_STEP 5

static const char PARAM_SEP[] = ",";
static const uint32_t VFR_OFFSET = 8;
static const char VFR_BACKET_START[] = "(";
static const char VFR_BRACKET_END[] = ")";
static const char FRAMERATE_COUNT = 10;

const int OMXCameraAdapter::SENSORID_IMX060 = 300;
const int OMXCameraAdapter::SENSORID_OV5650 = 301;
const int OMXCameraAdapter::SENSORID_OV5640 = 302;
const int OMXCameraAdapter::SENSORID_S5K4E1GA = 305;
const int OMXCameraAdapter::SENSORID_S5K6A1GX03 = 306;

/**** look up tables to translate OMX Caps to Parameter ****/

const CapResolution OMXCameraAdapter::mImageCapRes [] = {
    { 4032, 3024, "4032x3024" },
    { 4000, 3000, "4000x3000" },
    { 3648, 2736, "3648x2736" },
    { 3264, 2448, "3264x2448" },
    { 2592, 1944, "2592x1944" },
    { 2592, 1728, "2592x1728" },
    { 2592, 1458, "2592x1458" },
    { 2304, 1296, "2304x1296" },
    { 2240, 1344, "2240x1344" },
    { 2160, 1440, "2160x1440" },
    { 2112, 1728, "2112x1728" },
    { 2016, 1512, "2016x1512" },
    { 2000, 1600, "2000x1600" },
    { 1600, 1200, "1600x1200" },
    { 1280, 1024, "1280x1024" },
    { 1152, 864, "1152x864" },
    { 1280, 960, "1280x960" },
    { 640, 480, "640x480" },
    { 320, 240, "320x240" },
};

const CapResolution OMXCameraAdapter::mPreviewRes [] = {
    { 1920, 1080, "1920x1080" },
    { 1280, 720, "1280x720" },
    { 960, 720, "960x720" },
    { 800, 480, "800x480" },
    { 720, 576, "720x576" },
    { 720, 480, "720x480" },
    { 768, 576, "768x576" },
    { 640, 480, "640x480" },
    { 320, 240, "320x240" },
    { 352, 288, "352x288" },
    { 240, 160, "240x160" },
    { 176, 144, "176x144" },
    { 160, 120, "160x120" },
    { 128, 96, "128x96" },
    //Portait resolutions
    { 1088, 1920, "1088x1920" },
    { 720, 1280, "720x1280" },
    { 480, 800, "480x800" },
    { 576, 720, "576x720" },
    { 576, 768, "576x768" },
    { 480, 720, "480x720" },
    { 480, 640, "480x640" },
    { 288, 352, "288x352" },
    { 240, 320, "240x320" },
    { 160, 240, "240x160" },
    { 144, 176, "144x176" },
    { 120, 160, "120x160"},
    { 96, 128, "96x128" }
};

const CapResolution OMXCameraAdapter::mThumbRes [] = {
    { 640, 480, "640x480" },
    { 160, 120, "160x120" },
    { 200, 120, "200x120" },
    { 320, 240, "320x240" },
    { 512, 384, "512x384" },
    { 352, 144, "352x144" },
    { 176, 144, "176x144" },
    { 96, 96, "96x96" },
};

const CapPixelformat OMXCameraAdapter::mPixelformats [] = {
    { OMX_COLOR_FormatUnused, TICameraParameters::PIXEL_FORMAT_UNUSED },
    { OMX_COLOR_FormatCbYCrY, CameraParameters::PIXEL_FORMAT_YUV422I },
    { OMX_COLOR_FormatYUV420SemiPlanar, CameraParameters::PIXEL_FORMAT_YUV420SP },
    { OMX_COLOR_Format16bitRGB565, CameraParameters::PIXEL_FORMAT_RGB565 },
    { OMX_COLOR_FormatRawBayer10bit, TICameraParameters::PIXEL_FORMAT_RAW },
    { OMX_COLOR_FormatYUV420SemiPlanar, CameraParameters::PIXEL_FORMAT_YUV420P },
};

const CapFramerate OMXCameraAdapter::mFramerates [] = {
    { 30, "30" },
    { 24, "24" },
    { 15, "15" },
};

const CapZoom OMXCameraAdapter::mZoomStages [] = {
    { 65536, "100" },
    { 68157, "104" },
    { 70124, "107" },
    { 72745, "111" },
    { 75366, "115" },
    { 77988, "119" },
    { 80609, "123" },
    { 83231, "127" },
    { 86508, "132" },
    { 89784, "137" },
    { 92406, "141" },
    { 95683, "146" },
    { 99615, "152" },
    { 102892, "157" },
    { 106168, "162" },
    { 110100, "168" },
    { 114033, "174" },
    { 117965, "180" },
    { 122552, "187" },
    { 126484, "193" },
    { 131072, "200" },
    { 135660, "207" },
    { 140247, "214" },
    { 145490, "222" },
    { 150733, "230" },
    { 155976, "238" },
    { 161219, "246" },
    { 167117, "255" },
    { 173015, "264" },
    { 178913, "273" },
    { 185467, "283" },
    { 192020, "293" },
    { 198574, "303" },
    { 205783, "314" },
    { 212992, "325" },
    { 220201, "336" },
    { 228065, "348" },
    { 236585, "361" },
    { 244449, "373" },
    { 252969, "386" },
    { 262144, "400" },
    { 271319, "414" },
    { 281149, "429" },
    { 290980, "444" },
    { 300810, "459" },
    { 311951, "476" },
    { 322437, "492" },
    { 334234, "510" },
    { 346030, "528" },
    { 357827, "546" },
    { 370934, "566" },
    { 384041, "586" },
    { 397148, "606" },
    { 411566, "628" },
    { 425984, "650" },
    { 441057, "673" },
    { 456131, "696" },
    { 472515, "721" },
    { 488899, "746" },
    { 506593, "773" },
    { 524288, "800" },
};

const CapISO OMXCameraAdapter::mISOStages [] = {
    { 0, "auto" },
    { 100, "100" },
    { 200, "200"},
    { 400, "400" },
    { 800, "800" },
    { 1000, "1000" },
    { 1200, "1200" },
    { 1600, "1600" },
};

// mapped values have to match with new_sensor_MSP.h
const CapU32 OMXCameraAdapter::mSensorNames [] = {
    { SENSORID_IMX060, "IMX060" },
    { SENSORID_OV5650, "OV5650" },
    { SENSORID_OV5640, "OV5640" },
    { SENSORID_S5K4E1GA, "S5K4E1GA"},
    { SENSORID_S5K6A1GX03, "S5K6A1GX03" }
    // TODO(XXX): need to account for S3D camera later
};

// values for supported variable framerates sorted in ascending order
// CapU32Pair = (max fps, min fps, string representation)
const CapU32Pair OMXCameraAdapter::mVarFramerates [] = {
    { 15, 15, "(15000,15000)"},
    { 24, 15, "(15000,24000)"},
    { 24, 24, "(24000,24000)"},
    { 27, 15, "(15000,27000)"},
    { 27, 24, "(24000,27000)"},
    { 27, 27, "(27000,27000)"},
    { 30, 15, "(15000,30000)"},
    { 30, 24, "(24000,30000)"},
    { 30, 30, "(30000,30000)" },
};

const userToOMX_LUT OMXCameraAdapter::mAutoConvergence [] = {
    { TICameraParameters::AUTOCONVERGENCE_MODE_DISABLE, OMX_TI_AutoConvergenceModeDisable },
    { TICameraParameters::AUTOCONVERGENCE_MODE_FRAME,   OMX_TI_AutoConvergenceModeFrame },
    { TICameraParameters::AUTOCONVERGENCE_MODE_CENTER,  OMX_TI_AutoConvergenceModeCenter },
    { TICameraParameters::AUTOCONVERGENCE_MODE_TOUCH,   OMX_TI_AutoConvergenceModeFocusFaceTouch },
    { TICameraParameters::AUTOCONVERGENCE_MODE_MANUAL,  OMX_TI_AutoConvergenceModeManual },
};

const LUTtype OMXCameraAdapter::mAutoConvergenceLUT = {
    ARRAY_SIZE(mAutoConvergence),
    mAutoConvergence
};

// values for supported camera facing direction
const CapU32 OMXCameraAdapter::mFacing [] = {
    { OMX_TI_SENFACING_BACK , TICameraParameters::FACING_BACK },
    { OMX_TI_SENFACING_FRONT, TICameraParameters::FACING_FRONT},
};

/*****************************************
 * internal static function declarations
 *****************************************/

/**** Utility functions to help translate OMX Caps to Parameter ****/

status_t OMXCameraAdapter::encodePixelformatCap(OMX_COLOR_FORMATTYPE format,
                              const CapPixelformat *cap,
                              size_t capCount,
                              char * buffer,
                              size_t bufferSize)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( ( NULL == buffer ) || ( NULL == cap ) ) {
        CAMHAL_LOGEA("Invalid input arguments");
        return -EINVAL;
    }


    for ( unsigned int i = 0 ; i < capCount ; i++ )
        {
        if ( format == cap[i].pixelformat )
            {
            if (buffer[0] != '\0') {
                strncat(buffer, PARAM_SEP, bufferSize - 1);
            }
            strncat(buffer, cap[i].param, bufferSize - 1);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::encodeFramerateCap(OMX_U32 framerateMax,
                            OMX_U32 framerateMin,
                            const CapFramerate *cap,
                            size_t capCount,
                            char * buffer,
                            size_t bufferSize)
{
    status_t ret = NO_ERROR;
    char tmpBuffer[FRAMERATE_COUNT];

    LOG_FUNCTION_NAME;

    if ( ( NULL == buffer ) || ( NULL == cap ) ) {
        CAMHAL_LOGEA("Invalid input arguments");
        return -EINVAL;
    }

    memset(tmpBuffer, '\0', FRAMERATE_COUNT);
    snprintf(tmpBuffer, FRAMERATE_COUNT - 1, "%lu", framerateMax);
    strncat(buffer, tmpBuffer, bufferSize - 1);

    for ( unsigned int i = 0; i < capCount; i++ ) {
        if ( (framerateMax > cap[i].num) && (framerateMin < cap[i].num) ) {
            strncat(buffer, PARAM_SEP, bufferSize - 1);
            strncat(buffer, cap[i].param, bufferSize - 1);
        }
    }

    memset(tmpBuffer, '\0', FRAMERATE_COUNT);
    if ( FPS_MIN < framerateMin ) {
        snprintf(tmpBuffer, FRAMERATE_COUNT - 1, "%lu", framerateMin);
    } else {
        snprintf(tmpBuffer, FRAMERATE_COUNT - 1, "%d", FPS_MIN);
    }
    strncat(buffer, PARAM_SEP, bufferSize - 1);
    strncat(buffer, tmpBuffer, bufferSize - 1);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::encodeVFramerateCap(OMX_TI_CAPTYPE &caps,
                                               const CapU32Pair *cap,
                                               size_t capCount,
                                               char *buffer,
                                               char *defaultRange,
                                               size_t bufferSize)
{
    status_t ret = NO_ERROR;
    uint32_t minVFR, maxVFR;
    int default_index = -1;

    LOG_FUNCTION_NAME;

    if ( (NULL == buffer) || (NULL == defaultRange) || (NULL == cap) ) {
        CAMHAL_LOGEA("Invalid input arguments");
        return -EINVAL;
    }

    if(caps.ulPrvVarFPSModesCount < 1) {
        return NO_ERROR;
    }

    // Assumption: last range in tPrvVarFPSModes will be for S30FPSHD mode
    minVFR = caps.tPrvVarFPSModes[caps.ulPrvVarFPSModesCount-1].nVarFPSMin >> VFR_OFFSET;
    maxVFR = caps.tPrvVarFPSModes[caps.ulPrvVarFPSModesCount-1].nVarFPSMax >> VFR_OFFSET;

    if (minVFR < FPS_MIN) {
        minVFR = FPS_MIN;
    }

    for (unsigned int i = 0; i < capCount; i++) {
        // add cap[i] if it is in range and maxVFR != minVFR
        if ((maxVFR >= cap[i].num1) && (minVFR <= cap[i].num2)) {
            if (buffer[0] != '\0') {
                strncat(buffer, PARAM_SEP, bufferSize - 1);
            }
            strncat(buffer, cap[i].param, bufferSize - 1);

            // choose the max variable framerate as default
            if (cap[i].num1 != cap[i].num2) {
                default_index = i;
            }
        }
    }

    // if we haven't found any caps in the list to populate
    // just use the min and max
    if (buffer[0] == '\0') {
        snprintf(buffer, bufferSize - 1,
             "(%u%s%u)",
             minVFR * CameraHal::VFR_SCALE,
             PARAM_SEP,
             maxVFR * CameraHal::VFR_SCALE);
    }

    if (default_index != -1) {
        snprintf(defaultRange, (MAX_PROP_VALUE_LENGTH - 1), "%lu%s%lu",
                 cap[default_index].num2 * CameraHal::VFR_SCALE,
                 PARAM_SEP,
                 cap[default_index].num1 * CameraHal::VFR_SCALE);
    } else {
        snprintf(defaultRange, (MAX_PROP_VALUE_LENGTH - 1), "%u%s%u",
                 minVFR * CameraHal::VFR_SCALE,
                 PARAM_SEP,
                 maxVFR * CameraHal::VFR_SCALE);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

size_t OMXCameraAdapter::encodeZoomCap(OMX_S32 maxZoom,
                     const CapZoom *cap,
                     size_t capCount,
                     char * buffer,
                     size_t bufferSize)
{
    status_t res = NO_ERROR;
    size_t ret = 0;

    LOG_FUNCTION_NAME;

    if ( (NULL == buffer) || (NULL == cap) ) {
        CAMHAL_LOGEA("Invalid input arguments");
        return -EINVAL;
    }


    for ( unsigned int i = 0; i < capCount; i++ ) {
        if ( cap[i].num <= maxZoom ) {
            if (buffer[0] != '\0') {
                strncat(buffer, PARAM_SEP, bufferSize - 1);
            }
            strncat(buffer, cap[i].param, bufferSize - 1);
            ret++;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::encodeISOCap(OMX_U32 maxISO,
                      const CapISO *cap,
                      size_t capCount,
                      char * buffer,
                      size_t bufferSize)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( (NULL == buffer) || (NULL == cap) ) {
        CAMHAL_LOGEA("Invalid input arguments");
        return -EINVAL;
    }

    for ( unsigned int i = 0; i < capCount; i++ ) {
        if ( cap[i].num <= maxISO) {
            if (buffer[0] != '\0') {
                strncat(buffer, PARAM_SEP, bufferSize - 1);
            }
            strncat(buffer, cap[i].param, bufferSize - 1);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::encodeSizeCap(OMX_TI_CAPRESTYPE &res,
                       const CapResolution *cap,
                       size_t capCount,
                       char * buffer,
                       size_t bufferSize)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( (NULL == buffer) || (NULL == cap) ) {
        CAMHAL_LOGEA("Invalid input arguments");
        return -EINVAL;
    }

    for ( unsigned int i = 0 ; i < capCount ; i++ ) {
        if ( (cap[i].width <= res.nWidthMax) &&
             (cap[i].height <= res.nHeightMax) &&
             (cap[i].width >= res.nWidthMin) &&
             (cap[i].height >= res.nHeightMin) ) {
                if (buffer[0] != '\0') {
                    strncat(buffer, PARAM_SEP, bufferSize - 1);
                }
                strncat(buffer, cap[i].param, bufferSize -1);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertImageSizes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    ret = encodeSizeCap(caps.tImageResRange,
                        mImageCapRes,
                        ARRAY_SIZE(mImageCapRes),
                        supported,
                        MAX_PROP_VALUE_LENGTH);

    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("Error inserting supported picture sizes 0x%x", ret);
    } else {
        params->set(CameraProperties::SUPPORTED_PICTURE_SIZES, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertPreviewSizes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    ret = encodeSizeCap(caps.tPreviewResRange,
                        mPreviewRes,
                        ARRAY_SIZE(mPreviewRes),
                        supported,
                        MAX_PROP_VALUE_LENGTH);

    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("Error inserting supported Landscape preview sizes 0x%x", ret);
        return ret;
    }

    /* Insert Portait Resolutions by verifying Potrait Capability Support */
    ret = encodeSizeCap(caps.tRotatedPreviewResRange,
                        mPreviewRes,
                        ARRAY_SIZE(mPreviewRes),
                        supported,
                        MAX_PROP_VALUE_LENGTH);

    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("Error inserting supported Potrait preview sizes 0x%x", ret);
    } else {
        params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertVideoSizes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    ret = encodeSizeCap(caps.tPreviewResRange,
                        mPreviewRes,
                        ARRAY_SIZE(mPreviewRes),
                        supported,
                        MAX_PROP_VALUE_LENGTH);

    if ( NO_ERROR != ret ) {
      CAMHAL_LOGEB("Error inserting supported video sizes 0x%x", ret);
    } else {
      params->set(CameraProperties::SUPPORTED_VIDEO_SIZES, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertThumbSizes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    ret = encodeSizeCap(caps.tThumbResRange,
                        mThumbRes,
                        ARRAY_SIZE(mThumbRes),
                        supported,
                        MAX_PROP_VALUE_LENGTH);

    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("Error inserting supported thumbnail sizes 0x%x", ret);
    } else {
        //CTS Requirement: 0x0 should always be supported
        if (supported[0] != '\0') {
            strncat(supported, PARAM_SEP, 1);
        }
        strncat(supported, "0x0", MAX_PROP_NAME_LENGTH);
        params->set(CameraProperties::SUPPORTED_THUMBNAIL_SIZES, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertZoomStages(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    size_t zoomStageCount = 0;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    zoomStageCount = encodeZoomCap(caps.xMaxWidthZoom,
                                   mZoomStages,
                                   ARRAY_SIZE(mZoomStages),
                                   supported,
                                   MAX_PROP_VALUE_LENGTH);

    params->set(CameraProperties::SUPPORTED_ZOOM_RATIOS, supported);
    params->set(CameraProperties::SUPPORTED_ZOOM_STAGES, zoomStageCount - 1); //As per CTS requirement

    if ( 0 == zoomStageCount ) {
        params->set(CameraProperties::ZOOM_SUPPORTED, TICameraParameters::ZOOM_UNSUPPORTED);
        params->set(CameraProperties::SMOOTH_ZOOM_SUPPORTED, TICameraParameters::ZOOM_UNSUPPORTED);
    } else {
        params->set(CameraProperties::ZOOM_SUPPORTED, TICameraParameters::ZOOM_SUPPORTED);
        params->set(CameraProperties::SMOOTH_ZOOM_SUPPORTED, TICameraParameters::ZOOM_SUPPORTED);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertImageFormats(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( int i = 0 ; i < caps.ulImageFormatCount ; i++ ) {
        ret = encodePixelformatCap(caps.eImageFormats[i],
                                   mPixelformats,
                                   ARRAY_SIZE(mPixelformats),
                                   supported,
                                   MAX_PROP_VALUE_LENGTH);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Error inserting supported picture formats 0x%x", ret);
            break;
        }
    }

    if ( NO_ERROR == ret ) {
        //jpeg is not supported in OMX capabilies yet
        if (supported[0] != '\0') {
            strncat(supported, PARAM_SEP, 1);
        }
        strncat(supported, CameraParameters::PIXEL_FORMAT_JPEG, MAX_PROP_VALUE_LENGTH - 1);
        strcat(supported, ",");
        strncat(supported, TICameraParameters::PIXEL_FORMAT_RAW_JPEG, MAX_PROP_VALUE_LENGTH - 1);
        params->set(CameraProperties::SUPPORTED_PICTURE_FORMATS, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertPreviewFormats(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( int i = 0 ; i < caps.ulPreviewFormatCount; i++ ) {
        ret = encodePixelformatCap(caps.ePreviewFormats[i],
                                   mPixelformats,
                                   ARRAY_SIZE(mPixelformats),
                                   supported,
                                   MAX_PROP_VALUE_LENGTH);
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Error inserting supported preview formats 0x%x", ret);
            break;
        }
    }

    if ( NO_ERROR == ret ) {
        // need to advertise we support YV12 format
        // We will program preview port with NV21 when we see application set YV12
        if (supported[0] != '\0') {
            strncat(supported, PARAM_SEP, 1);
        }
        strncat(supported, CameraParameters::PIXEL_FORMAT_YUV420P, MAX_PROP_VALUE_LENGTH - 1);
        params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertFramerates(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    ret = encodeFramerateCap(caps.xFramerateMax >> VFR_OFFSET,
                             caps.xFramerateMin >> VFR_OFFSET,
                             mFramerates,
                             ARRAY_SIZE(mFramerates),
                             supported,
                             MAX_PROP_VALUE_LENGTH);

    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("Error inserting supported preview framerates 0x%x", ret);
    } else {
        params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertVFramerates(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    char defaultRange[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    memset(defaultRange, '\0', MAX_PROP_VALUE_LENGTH);

    ret = encodeVFramerateCap(caps,
                              mVarFramerates,
                              ARRAY_SIZE(mVarFramerates),
                              supported,
                              defaultRange,
                              MAX_PROP_VALUE_LENGTH);

    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("Error inserting supported preview framerate ranges 0x%x", ret);
    } else {
        params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, supported);
        CAMHAL_LOGDB("Supported framerate ranges: %s", supported);
        params->set(CameraProperties::FRAMERATE_RANGE, defaultRange);
        params->set(CameraProperties::FRAMERATE_RANGE_VIDEO, defaultRange);
        params->set(CameraProperties::FRAMERATE_RANGE_IMAGE, defaultRange);
        CAMHAL_LOGDB("Default framerate range: [%s]", defaultRange);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertEVs(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    snprintf(supported, MAX_PROP_VALUE_LENGTH, "%d", ( int ) ( caps.xEVCompensationMin * 10 ));
    params->set(CameraProperties::SUPPORTED_EV_MIN, supported);

    snprintf(supported, MAX_PROP_VALUE_LENGTH, "%d", ( int ) ( caps.xEVCompensationMax * 10 ));
    params->set(CameraProperties::SUPPORTED_EV_MAX, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertISOModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    ret = encodeISOCap(caps.nSensitivityMax,
                       mISOStages,
                       ARRAY_SIZE(mISOStages),
                       supported,
                       MAX_PROP_VALUE_LENGTH);
    if ( NO_ERROR != ret ) {
        CAMHAL_LOGEB("Error inserting supported ISO modes 0x%x", ret);
    } else {
        params->set(CameraProperties::SUPPORTED_ISO_VALUES, supported);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertIPPModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    //Off is always supported
    strncat(supported, TICameraParameters::IPP_NONE, MAX_PROP_NAME_LENGTH);

    if ( caps.bLensDistortionCorrectionSupported ) {
        strncat(supported, PARAM_SEP, 1);
        strncat(supported, TICameraParameters::IPP_LDC, MAX_PROP_NAME_LENGTH);
    }

    if ( caps.bISONoiseFilterSupported ) {
        strncat(supported, PARAM_SEP, 1);
        strncat(supported, TICameraParameters::IPP_NSF, MAX_PROP_NAME_LENGTH);
    }

    if ( caps.bISONoiseFilterSupported && caps.bLensDistortionCorrectionSupported ) {
        strncat(supported, PARAM_SEP, 1);
        strncat(supported, TICameraParameters::IPP_LDCNSF, MAX_PROP_NAME_LENGTH);
    }

    params->set(CameraProperties::SUPPORTED_IPP_MODES, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertWBModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( unsigned int i = 0 ; i < caps.ulWhiteBalanceCount ; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eWhiteBalanceModes[i], WBalLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            strncat(supported, p, MAX_PROP_NAME_LENGTH);
        }
    }

    params->set(CameraProperties::SUPPORTED_WHITE_BALANCE, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertEffects(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( unsigned int i = 0 ; i < caps.ulColorEffectCount; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eColorEffects[i], EffLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            strncat(supported, p, MAX_PROP_NAME_LENGTH);
        }
    }

    params->set(CameraProperties::SUPPORTED_EFFECTS, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertExpModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( unsigned int i = 0 ; i < caps.ulExposureModeCount; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eExposureModes[i], ExpLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            strncat(supported, p, MAX_PROP_NAME_LENGTH);
        }
    }

    params->set(CameraProperties::SUPPORTED_EXPOSURE_MODES, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertFlashModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( unsigned int i = 0 ; i < caps.ulFlashCount; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eFlashModes[i], FlashLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            strncat(supported, p, MAX_PROP_NAME_LENGTH);
        }
    }

    if ( strlen(supported) == 0 ) {
        strncpy(supported, DEFAULT_FLASH_MODE, MAX_PROP_NAME_LENGTH);
    }

    params->set(CameraProperties::SUPPORTED_FLASH_MODES, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertSceneModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( unsigned int i = 0 ; i < caps.ulSceneCount; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eSceneModes[i], SceneLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            strncat(supported, p, MAX_PROP_NAME_LENGTH);
        }
    }

    params->set(CameraProperties::SUPPORTED_SCENE_MODES, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertFocusModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( unsigned int i = 0 ; i < caps.ulFocusModeCount; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eFocusModes[i], FocusLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            strncat(supported, p, MAX_PROP_NAME_LENGTH);
        }
    }

    if (supported[0] != '\0') {
        strncat(supported, PARAM_SEP, 1);
    }

    // Check if focus is supported by camera
    if (caps.ulFocusModeCount == 1 &&
        caps.eFocusModes[0] == OMX_IMAGE_FocusControlOff) {
        // Focus is not supported by camera
        // Advertise this to app as infinitiy focus mode
        strncat(supported, CameraParameters::FOCUS_MODE_INFINITY, MAX_PROP_NAME_LENGTH);
    } else {
        // Focus is supported but these modes are not supported by the
        // capability feature. Apply manually
        strncat(supported, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE, MAX_PROP_NAME_LENGTH);
    }

    params->set(CameraProperties::SUPPORTED_FOCUS_MODES, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertFlickerModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    for ( unsigned int i = 0 ; i < caps.ulFlickerCount; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eFlicker[i], FlickerLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, 1);
            }
            strncat(supported, p, MAX_PROP_NAME_LENGTH);
        }
    }

    params->set(CameraProperties::SUPPORTED_ANTIBANDING, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertAreas(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);

    sprintf(supported, "%d", caps.ulAlgoAreasFocusCount);
    params->set(CameraProperties::MAX_FOCUS_AREAS, supported);
    CAMHAL_LOGDB("Maximum supported focus areas %s", supported);

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    sprintf(supported, "%d", caps.ulAlgoAreasExposureCount);
    params->set(CameraProperties::MAX_NUM_METERING_AREAS, supported);
    CAMHAL_LOGDB("Maximum supported exposure areas %s", supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertLocks(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if ( caps.bAELockSupported ) {
        params->set(CameraProperties::AUTO_EXPOSURE_LOCK_SUPPORTED, DEFAULT_LOCK_SUPPORTED);
    }

    if ( caps.bAWBLockSupported ) {
        params->set(CameraProperties::AUTO_WHITEBALANCE_LOCK_SUPPORTED, DEFAULT_LOCK_SUPPORTED);
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::insertSenMount(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;
    unsigned int i = 0;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', sizeof(supported));

    // 1) Look up and assign sensor name
    for (i = 0; i < ARRAY_SIZE(mSensorNames); i++) {
        if(mSensorNames[i].num == caps.tSenMounting.nSenId) {
            // sensor found
            break;
        }
    }
    if ( i == ARRAY_SIZE(mSensorNames) ) {
        p = "UNKNOWN_SENSOR";
    } else {
        p = mSensorNames[i].param;
    }
    strncat(supported, p, REMAINING_BYTES(supported));
    params->set(CameraProperties::CAMERA_NAME, supported);
    params->set(CameraProperties::CAMERA_SENSOR_ID, caps.tSenMounting.nSenId);

    // 2) Assign mounting rotation
    params->set(CameraProperties::ORIENTATION_INDEX, caps.tSenMounting.nRotation);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertFacing(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;
    unsigned int i = 0;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', sizeof(supported));

    for (i = 0; i < ARRAY_SIZE(mFacing); i++) {
        if((OMX_TI_SENFACING_TYPE)mFacing[i].num == caps.tSenMounting.eFacing) {
            break;
        }
    }
    if ( i == ARRAY_SIZE(mFacing) ) {
        p = "UNKNOWN_FACING";
    } else {
        p = mFacing[i].param;
    }
    strncat(supported, p, REMAINING_BYTES(supported));
    params->set(CameraProperties::FACING_INDEX, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertFocalLength(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    memset(supported, '\0', sizeof(supported));

    sprintf(supported, "%d", caps.nFocalLength / 100);
    strncat(supported, ".", REMAINING_BYTES(supported));
    sprintf(supported+(strlen(supported)*sizeof(char)), "%d", caps.nFocalLength % 100);

    params->set(CameraProperties::FOCAL_LENGTH, supported);

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::insertAutoConvergenceModes(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];
    const char *p;
    unsigned int i = 0;

    LOG_FUNCTION_NAME;

    memset(supported, '\0', sizeof(supported));

    for ( unsigned int i = 0 ; i < caps.ulAutoConvModesCount; i++ ) {
        p = getLUTvalue_OMXtoHAL(caps.eAutoConvModes[i], mAutoConvergenceLUT);
        if ( NULL != p ) {
            if (supported[0] != '\0') {
                strncat(supported, PARAM_SEP, REMAINING_BYTES(supported));
            }
            strncat(supported, p, REMAINING_BYTES(supported));
        }
    }
    params->set(CameraProperties::AUTOCONVERGENCE_MODE_VALUES, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertManualConvergenceRange(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    char supported[MAX_PROP_VALUE_LENGTH];

    LOG_FUNCTION_NAME;

    snprintf(supported, MAX_PROP_VALUE_LENGTH, "%d", ( int ) ( caps.nManualConvMin ));
    params->set(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MIN, supported);

    snprintf(supported, MAX_PROP_VALUE_LENGTH, "%d", ( int ) ( caps.nManualConvMax ));
    params->set(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_MAX, supported);

    snprintf(supported, MAX_PROP_VALUE_LENGTH, "%d", ( int ) ( caps.nManualConvMax != caps.nManualConvMin ));
    params->set(CameraProperties::SUPPORTED_MANUAL_CONVERGENCE_STEP, supported);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertDefaults(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    const char *p;

    LOG_FUNCTION_NAME;

    params->set(CameraProperties::ANTIBANDING, DEFAULT_ANTIBANDING);
    params->set(CameraProperties::BRIGHTNESS, DEFAULT_BRIGHTNESS);
    params->set(CameraProperties::CONTRAST, DEFAULT_CONTRAST);
    params->set(CameraProperties::EFFECT, DEFAULT_EFFECT);
    params->set(CameraProperties::EV_COMPENSATION, DEFAULT_EV_COMPENSATION);
    params->set(CameraProperties::SUPPORTED_EV_STEP, DEFAULT_EV_STEP);
    params->set(CameraProperties::EXPOSURE_MODE, DEFAULT_EXPOSURE_MODE);
    params->set(CameraProperties::FLASH_MODE, DEFAULT_FLASH_MODE);
    char *pos = strstr(params->get(CameraProperties::SUPPORTED_FOCUS_MODES), DEFAULT_FOCUS_MODE_PREFERRED);
    if ( NULL != pos )
        {
        params->set(CameraProperties::FOCUS_MODE, DEFAULT_FOCUS_MODE_PREFERRED);
        }
    else
        {
        params->set(CameraProperties::FOCUS_MODE, DEFAULT_FOCUS_MODE);
        }
    params->set(CameraProperties::IPP, DEFAULT_IPP);
    params->set(CameraProperties::GBCE, DEFAULT_GBCE);
    params->set(CameraProperties::ISO_MODE, DEFAULT_ISO_MODE);
    params->set(CameraProperties::JPEG_QUALITY, DEFAULT_JPEG_QUALITY);
    params->set(CameraProperties::JPEG_THUMBNAIL_QUALITY, DEFAULT_THUMBNAIL_QUALITY);
    params->set(CameraProperties::JPEG_THUMBNAIL_SIZE, DEFAULT_THUMBNAIL_SIZE);
    params->set(CameraProperties::PICTURE_FORMAT, DEFAULT_PICTURE_FORMAT);
    params->set(CameraProperties::PICTURE_SIZE, DEFAULT_PICTURE_SIZE);
    params->set(CameraProperties::PREVIEW_FORMAT, DEFAULT_PREVIEW_FORMAT);
    params->set(CameraProperties::PREVIEW_FRAME_RATE, DEFAULT_FRAMERATE);
    params->set(CameraProperties::PREVIEW_SIZE, DEFAULT_PREVIEW_SIZE);
    params->set(CameraProperties::REQUIRED_PREVIEW_BUFS, DEFAULT_NUM_PREV_BUFS);
    params->set(CameraProperties::REQUIRED_IMAGE_BUFS, DEFAULT_NUM_PIC_BUFS);
    params->set(CameraProperties::SATURATION, DEFAULT_SATURATION);
    params->set(CameraProperties::SCENE_MODE, DEFAULT_SCENE_MODE);
    params->set(CameraProperties::SHARPNESS, DEFAULT_SHARPNESS);
    params->set(CameraProperties::VSTAB, DEFAULT_VSTAB);
    params->set(CameraProperties::VSTAB_SUPPORTED, DEFAULT_VSTAB_SUPPORTED);
    params->set(CameraProperties::WHITEBALANCE, DEFAULT_WB);
    params->set(CameraProperties::ZOOM, DEFAULT_ZOOM);
    params->set(CameraProperties::MAX_FD_HW_FACES, DEFAULT_MAX_FD_HW_FACES);
    params->set(CameraProperties::MAX_FD_SW_FACES, DEFAULT_MAX_FD_SW_FACES);
    params->set(CameraProperties::AUTO_EXPOSURE_LOCK, DEFAULT_AE_LOCK);
    params->set(CameraProperties::AUTO_WHITEBALANCE_LOCK, DEFAULT_AWB_LOCK);
    params->set(CameraProperties::HOR_ANGLE, DEFAULT_HOR_ANGLE);
    params->set(CameraProperties::VER_ANGLE, DEFAULT_VER_ANGLE);
    params->set(CameraProperties::VIDEO_SNAPSHOT_SUPPORTED, DEFAULT_VIDEO_SNAPSHOT_SUPPORTED);
    params->set(CameraProperties::VIDEO_SIZE, DEFAULT_VIDEO_SIZE);
    params->set(CameraProperties::PREFERRED_PREVIEW_SIZE_FOR_VIDEO, DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO);
    params->set(CameraProperties::SENSOR_ORIENTATION, DEFAULT_SENSOR_ORIENTATION);
    params->set(CameraProperties::AUTOCONVERGENCE_MODE, DEFAULT_AUTOCONVERGENCE_MODE);
    params->set(CameraProperties::MANUAL_CONVERGENCE, DEFAULT_MANUAL_CONVERGENCE);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::insertCapabilities(CameraProperties::Properties* params, OMX_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret ) {
        ret = insertImageSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertPreviewSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertThumbSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertZoomStages(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertImageFormats(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertPreviewFormats(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertFramerates(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertVFramerates(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertEVs(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertISOModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertIPPModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertWBModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertEffects(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertExpModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertFlashModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertSceneModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertFocusModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertFlickerModes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertSenMount(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertLocks(params, caps);
    }

    if ( NO_ERROR == ret) {
        ret = insertAreas(params, caps);
    }

    if ( NO_ERROR == ret) {
        ret = insertFacing(params, caps);
    }

    if ( NO_ERROR == ret) {
        ret = insertFocalLength(params, caps);
    }

    if ( NO_ERROR == ret) {
        ret = insertAutoConvergenceModes(params, caps);
    }

    if ( NO_ERROR == ret) {
        ret = insertManualConvergenceRange(params, caps);
    }

    //NOTE: Ensure that we always call insertDefaults after inserting the supported capabilities
    //as there are checks inside insertDefaults to make sure a certain default is supported
    // or not
    if ( NO_ERROR == ret ) {
        ret = insertVideoSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertDefaults(params, caps);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


bool OMXCameraAdapter::_checkOmxTiCap(const OMX_TI_CAPTYPE & caps)
{
#define CAMHAL_CHECK_OMX_TI_CAP(countVar, arrayVar)              \
    do {                                                         \
        const int count = static_cast<int>(caps.countVar);  \
        const int maxSize = CAMHAL_SIZE_OF_ARRAY(caps.arrayVar); \
        if ( count < 0 || count > maxSize )                      \
        {                                                        \
            CAMHAL_LOGE("OMX_TI_CAPTYPE verification failed");   \
            CAMHAL_LOGE("  variable: OMX_TI_CAPTYPE::" #countVar \
                    ", value: %d, max allowed: %d",              \
                    count, maxSize);                             \
            return false;                                        \
        }                                                        \
    } while (0)

    CAMHAL_CHECK_OMX_TI_CAP(ulPreviewFormatCount, ePreviewFormats);
    CAMHAL_CHECK_OMX_TI_CAP(ulImageFormatCount, eImageFormats);
    CAMHAL_CHECK_OMX_TI_CAP(ulWhiteBalanceCount, eWhiteBalanceModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulColorEffectCount, eColorEffects);
    CAMHAL_CHECK_OMX_TI_CAP(ulFlickerCount, eFlicker);
    CAMHAL_CHECK_OMX_TI_CAP(ulExposureModeCount, eExposureModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulFocusModeCount, eFocusModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulSceneCount, eSceneModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulFlashCount, eFlashModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulPrvVarFPSModesCount, tPrvVarFPSModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulCapVarFPSModesCount, tCapVarFPSModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulAutoConvModesCount, eAutoConvModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulBracketingModesCount, eBracketingModes);
    CAMHAL_CHECK_OMX_TI_CAP(ulImageCodingFormatCount, eImageCodingFormat);
    CAMHAL_CHECK_OMX_TI_CAP(ulPrvFrameLayoutCount, ePrvFrameLayout);
    CAMHAL_CHECK_OMX_TI_CAP(ulCapFrameLayoutCount, eCapFrameLayout);

#undef CAMHAL_CHECK_OMX_TI_CAP

    return true;
}


bool OMXCameraAdapter::_dumpOmxTiCap(const int sensorId, const OMX_TI_CAPTYPE & caps)
{
    if ( !_checkOmxTiCap(caps) )
    {
        CAMHAL_LOGE("OMX_TI_CAPTYPE structure is invalid");
        return false;
    }

    CAMHAL_LOGD("===================================================");
    CAMHAL_LOGD("---- Dumping OMX capabilities for sensor id: %d ----", sensorId);

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulPreviewFormatCount = %d", int(caps.ulPreviewFormatCount));
    for ( int i = 0; i < int(caps.ulPreviewFormatCount); ++i )
        CAMHAL_LOGD("  ePreviewFormats[%2d] = %d", i, int(caps.ePreviewFormats[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulImageFormatCount = %d", int(caps.ulImageFormatCount));
    for ( int i = 0; i < int(caps.ulImageFormatCount); ++i )
        CAMHAL_LOGD("  eImageFormats[%2d] = %d", i, int(caps.eImageFormats[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("tPreviewResRange.nWidthMin  = %d", int(caps.tPreviewResRange.nWidthMin));
    CAMHAL_LOGD("tPreviewResRange.nHeightMin = %d", int(caps.tPreviewResRange.nHeightMin));
    CAMHAL_LOGD("tPreviewResRange.nWidthMax  = %d", int(caps.tPreviewResRange.nWidthMax));
    CAMHAL_LOGD("tPreviewResRange.nHeightMax = %d", int(caps.tPreviewResRange.nHeightMax));
    CAMHAL_LOGD("tPreviewResRange.nMaxResInPixels = %d", int(caps.tPreviewResRange.nMaxResInPixels));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("tRotatedPreviewResRange.nWidthMin  = %d", int(caps.tRotatedPreviewResRange.nWidthMin));
    CAMHAL_LOGD("tRotatedPreviewResRange.nHeightMin = %d", int(caps.tRotatedPreviewResRange.nHeightMin));
    CAMHAL_LOGD("tRotatedPreviewResRange.nWidthMax  = %d", int(caps.tRotatedPreviewResRange.nWidthMax));
    CAMHAL_LOGD("tRotatedPreviewResRange.nHeightMax = %d", int(caps.tRotatedPreviewResRange.nHeightMax));
    CAMHAL_LOGD("tRotatedPreviewResRange.nMaxResInPixels = %d", int(caps.tRotatedPreviewResRange.nMaxResInPixels));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("tImageResRange.nWidthMin  = %d", int(caps.tImageResRange.nWidthMin));
    CAMHAL_LOGD("tImageResRange.nHeightMin = %d", int(caps.tImageResRange.nHeightMin));
    CAMHAL_LOGD("tImageResRange.nWidthMax  = %d", int(caps.tImageResRange.nWidthMax));
    CAMHAL_LOGD("tImageResRange.nHeightMax = %d", int(caps.tImageResRange.nHeightMax));
    CAMHAL_LOGD("tImageResRange.nMaxResInPixels = %d", int(caps.tImageResRange.nMaxResInPixels));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("tThumbResRange.nWidthMin  = %d", int(caps.tThumbResRange.nWidthMin));
    CAMHAL_LOGD("tThumbResRange.nHeightMin = %d", int(caps.tThumbResRange.nHeightMin));
    CAMHAL_LOGD("tThumbResRange.nWidthMax  = %d", int(caps.tThumbResRange.nWidthMax));
    CAMHAL_LOGD("tThumbResRange.nHeightMax = %d", int(caps.tThumbResRange.nHeightMax));
    CAMHAL_LOGD("tThumbResRange.nMaxResInPixels = %d", int(caps.tThumbResRange.nMaxResInPixels));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulWhiteBalanceCount = %d", int(caps.ulWhiteBalanceCount));
    for ( int i = 0; i < int(caps.ulWhiteBalanceCount); ++i )
        CAMHAL_LOGD("  eWhiteBalanceModes[%2d] = 0x%08x", i, int(caps.eWhiteBalanceModes[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulColorEffectCount = %d", int(caps.ulColorEffectCount));
    for ( int i = 0; i < int(caps.ulColorEffectCount); ++i )
        CAMHAL_LOGD("  eColorEffects[%2d] = 0x%08x", i, int(caps.eColorEffects[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("xMaxWidthZoom  = %d", int(caps.xMaxWidthZoom));
    CAMHAL_LOGD("xMaxHeightZoom = %d", int(caps.xMaxHeightZoom));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulFlickerCount = %d", int(caps.ulFlickerCount));
    for ( int i = 0; i < int(caps.ulFlickerCount); ++i )
        CAMHAL_LOGD("  eFlicker[%2d] = %d", i, int(caps.eFlicker[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulExposureModeCount = %d", int(caps.ulExposureModeCount));
    for ( int i = 0; i < int(caps.ulExposureModeCount); ++i )
        CAMHAL_LOGD("  eExposureModes[%2d] = 0x%08x", i, int(caps.eExposureModes[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("bLensDistortionCorrectionSupported = %d", int(caps.bLensDistortionCorrectionSupported));
    CAMHAL_LOGD("bISONoiseFilterSupported = %d", int(caps.bISONoiseFilterSupported));
    CAMHAL_LOGD("xEVCompensationMin = %d", int(caps.xEVCompensationMin));
    CAMHAL_LOGD("xEVCompensationMax = %d", int(caps.xEVCompensationMax));
    CAMHAL_LOGD("nSensitivityMax = %d", int(caps.nSensitivityMax));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulFocusModeCount = %d", int(caps.ulFocusModeCount));
    for ( int i = 0; i < int(caps.ulFocusModeCount); ++i )
        CAMHAL_LOGD("  eFocusModes[%2d] = 0x%08x", i, int(caps.eFocusModes[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulSceneCount = %d", int(caps.ulSceneCount));
    for ( int i = 0; i < int(caps.ulSceneCount); ++i )
        CAMHAL_LOGD("  eSceneModes[%2d] = %d", i, int(caps.eSceneModes[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulFlashCount = %d", int(caps.ulFlashCount));
    for ( int i = 0; i < int(caps.ulFlashCount); ++i )
        CAMHAL_LOGD("  eFlashModes[%2d] = %d", i, int(caps.eFlashModes[i]));

    CAMHAL_LOGD("xFramerateMin = %d", int(caps.xFramerateMin));
    CAMHAL_LOGD("xFramerateMax = %d", int(caps.xFramerateMax));
    CAMHAL_LOGD("bContrastSupported = %d", int(caps.bContrastSupported));
    CAMHAL_LOGD("bSaturationSupported = %d", int(caps.bSaturationSupported));
    CAMHAL_LOGD("bBrightnessSupported = %d", int(caps.bBrightnessSupported));
    CAMHAL_LOGD("bProcessingLevelSupported = %d", int(caps.bProcessingLevelSupported));
    CAMHAL_LOGD("bQFactorSupported = %d", int(caps.bQFactorSupported));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulPrvVarFPSModesCount = %d", int(caps.ulPrvVarFPSModesCount));
    for ( int i = 0; i < int(caps.ulPrvVarFPSModesCount); ++i )
    {
        CAMHAL_LOGD("  tPrvVarFPSModes[%d].nVarFPSMin = %d", i, int(caps.tPrvVarFPSModes[i].nVarFPSMin));
        CAMHAL_LOGD("  tPrvVarFPSModes[%d].nVarFPSMax = %d", i, int(caps.tPrvVarFPSModes[i].nVarFPSMax));
    }

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulCapVarFPSModesCount = %d", int(caps.ulCapVarFPSModesCount));
    for ( int i = 0; i < int(caps.ulCapVarFPSModesCount); ++i )
    {
        CAMHAL_LOGD("  tCapVarFPSModes[%d].nVarFPSMin = %d", i, int(caps.tCapVarFPSModes[i].nVarFPSMin));
        CAMHAL_LOGD("  tCapVarFPSModes[%d].nVarFPSMax = %d", i, int(caps.tCapVarFPSModes[i].nVarFPSMax));
    }

    CAMHAL_LOGD("");
    CAMHAL_LOGD("tSenMounting.nSenId    = %d", int(caps.tSenMounting.nSenId));
    CAMHAL_LOGD("tSenMounting.nRotation = %d", int(caps.tSenMounting.nRotation));
    CAMHAL_LOGD("tSenMounting.bMirror   = %d", int(caps.tSenMounting.bMirror));
    CAMHAL_LOGD("tSenMounting.bFlip     = %d", int(caps.tSenMounting.bFlip));
    CAMHAL_LOGD("tSenMounting.eFacing   = %d", int(caps.tSenMounting.eFacing));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulAutoConvModesCount = %d", int(caps.ulAutoConvModesCount));
    for ( int i = 0; i < int(caps.ulAutoConvModesCount); ++i )
        CAMHAL_LOGD("  eAutoConvModes[%2d] = %d", i, int(caps.eAutoConvModes[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulBracketingModesCount = %d", int(caps.ulBracketingModesCount));
    for ( int i = 0; i < int(caps.ulBracketingModesCount); ++i )
        CAMHAL_LOGD("  eBracketingModes[%2d] = %d", i, int(caps.eBracketingModes[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("bVidStabSupported = %d", int(caps.bVidStabSupported));
	CAMHAL_LOGD("bGbceSupported    = %d", int(caps.bGbceSupported));
    CAMHAL_LOGD("bRawJpegSupported = %d", int(caps.bRawJpegSupported));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulImageCodingFormatCount = %d", int(caps.ulImageCodingFormatCount));
    for ( int i = 0; i < int(caps.ulImageCodingFormatCount); ++i )
        CAMHAL_LOGD("  eImageCodingFormat[%2d] = %d", i, int(caps.eImageCodingFormat[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("uSenNativeResWidth       = %d", int(caps.uSenNativeResWidth));
    CAMHAL_LOGD("uSenNativeResHeight      = %d", int(caps.uSenNativeResHeight));
    CAMHAL_LOGD("ulAlgoAreasFocusCount    = %d", int(caps.ulAlgoAreasFocusCount));
    CAMHAL_LOGD("ulAlgoAreasExposureCount = %d", int(caps.ulAlgoAreasExposureCount));
    CAMHAL_LOGD("bAELockSupported         = %d", int(caps.bAELockSupported));
    CAMHAL_LOGD("bAWBLockSupported        = %d", int(caps.bAWBLockSupported));
    CAMHAL_LOGD("bAFLockSupported         = %d", int(caps.bAFLockSupported));
    CAMHAL_LOGD("nFocalLength             = %d", int(caps.nFocalLength));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulPrvFrameLayoutCount = %d", int(caps.ulPrvFrameLayoutCount));
    for ( int i = 0; i < int(caps.ulPrvFrameLayoutCount); ++i )
        CAMHAL_LOGD("  ePrvFrameLayout[%2d] = %d", i, int(caps.ePrvFrameLayout[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("ulCapFrameLayoutCount = %d", int(caps.ulCapFrameLayoutCount));
    for ( int i = 0; i < int(caps.ulCapFrameLayoutCount); ++i )
        CAMHAL_LOGD("  eCapFrameLayout[%2d] = %d", i, int(caps.eCapFrameLayout[i]));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("bVideoNoiseFilterSupported         = %d", int(caps.bVideoNoiseFilterSupported      ));
    CAMHAL_LOGD("bVideoStabilizationSupported       = %d", int(caps.bVideoStabilizationSupported    ));
    CAMHAL_LOGD("bStillCapDuringVideoSupported      = %d", int(caps.bStillCapDuringVideoSupported   ));
    CAMHAL_LOGD("bMechanicalMisalignmentSupported   = %d", int(caps.bMechanicalMisalignmentSupported));
    CAMHAL_LOGD("bFacePrioritySupported             = %d", int(caps.bFacePrioritySupported          ));
    CAMHAL_LOGD("bRegionPrioritySupported           = %d", int(caps.bRegionPrioritySupported        ));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("nManualConvMin     = %d", int(caps.nManualConvMin     ));
    CAMHAL_LOGD("nManualConvMax     = %d", int(caps.nManualConvMax     ));
    CAMHAL_LOGD("nManualExpMin      = %d", int(caps.nManualExpMin      ));
    CAMHAL_LOGD("nManualExpMax      = %d", int(caps.nManualExpMax      ));
    CAMHAL_LOGD("nBrightnessMin     = %d", int(caps.nBrightnessMin     ));
    CAMHAL_LOGD("nBrightnessMax     = %d", int(caps.nBrightnessMax     ));
    CAMHAL_LOGD("nContrastMin       = %d", int(caps.nContrastMin       ));
    CAMHAL_LOGD("nContrastMax       = %d", int(caps.nContrastMax       ));
    CAMHAL_LOGD("nSharpnessMin      = %d", int(caps.nSharpnessMin      ));
    CAMHAL_LOGD("nSharpnessMax      = %d", int(caps.nSharpnessMax      ));
    CAMHAL_LOGD("nSaturationMin     = %d", int(caps.nSaturationMin     ));
    CAMHAL_LOGD("nSaturationMax     = %d", int(caps.nSaturationMax     ));

    CAMHAL_LOGD("");
    CAMHAL_LOGD("------------------- end of dump -------------------");
    CAMHAL_LOGD("===================================================");

    return true;
}

/*****************************************
 * public exposed function declarations
 *****************************************/

status_t OMXCameraAdapter::getCaps(const int sensorId, CameraProperties::Properties* params, OMX_HANDLETYPE handle)
{
    status_t ret = NO_ERROR;
    int caps_size = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CAPTYPE** caps = NULL;;
    OMX_TI_CONFIG_SHAREDBUFFER sharedBuffer;
    MemoryManager memMgr;

    LOG_FUNCTION_NAME;

    // allocate tiler (or ion) buffer for caps (size is always a multiple of 4K)
    caps_size = ((sizeof(OMX_TI_CAPTYPE)+4095)/4096)*4096;
    caps = (OMX_TI_CAPTYPE**) memMgr.allocateBuffer(0, 0, NULL, caps_size, 1);

    if (!caps) {
        CAMHAL_LOGEB("Error allocating buffer for caps %d", eError);
        ret = -ENOMEM;
        goto EXIT;
    }

    // initialize structures to be passed to OMX Camera
    OMX_INIT_STRUCT_PTR (caps[0], OMX_TI_CAPTYPE);
    caps[0]->nPortIndex = OMX_ALL;

    OMX_INIT_STRUCT_PTR (&sharedBuffer, OMX_TI_CONFIG_SHAREDBUFFER);
    sharedBuffer.nPortIndex = OMX_ALL;
    sharedBuffer.nSharedBuffSize = caps_size;
    sharedBuffer.pSharedBuff = (OMX_U8 *) caps[0];

    // Get capabilities from OMX Camera
    eError =  OMX_GetConfig(handle, (OMX_INDEXTYPE) OMX_TI_IndexConfigCamCapabilities, &sharedBuffer);
    if ( OMX_ErrorNone != eError ) {
        CAMHAL_LOGEB("Error during capabilities query 0x%x", eError);
        ret = UNKNOWN_ERROR;
        goto EXIT;
    } else {
        CAMHAL_LOGDA("OMX capability query success");
    }

#ifdef CAMERAHAL_DEBUG
    _dumpOmxTiCap(sensorId, *caps[0]);
#endif

    // Translate and insert Ducati capabilities to CameraProperties
    if ( NO_ERROR == ret ) {
        ret = insertCapabilities(params, *caps[0]);
    }

    CAMHAL_LOGDB("sen mount id=%u", (unsigned int)caps[0]->tSenMounting.nSenId);
    CAMHAL_LOGDB("facing id=%u", (unsigned int)caps[0]->tSenMounting.eFacing);

 EXIT:
    if (caps) {
        memMgr.freeBuffer((void*) caps);
        caps = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

};
