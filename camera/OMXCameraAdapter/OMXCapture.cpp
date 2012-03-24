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
* @file OMXCapture.cpp
*
* This file contains functionality for handling image capture.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"


namespace android {

status_t OMXCameraAdapter::setParametersCapture(const CameraParameters &params,
                                                BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    int w, h;
    OMX_COLOR_FORMATTYPE pixFormat;
    CodingMode codingMode = mCodingMode;
    const char *valstr = NULL;
    OMX_TI_STEREOFRAMELAYOUTTYPE capFrmLayout;

    LOG_FUNCTION_NAME;

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    capFrmLayout = cap->mFrameLayoutType;
    setParamS3D(mCameraAdapterParameters.mImagePortIndex,
        params.get(TICameraParameters::KEY_S3D_CAP_FRAME_LAYOUT));
    if (capFrmLayout != cap->mFrameLayoutType) {
        mPendingCaptureSettings |= SetFormat;
    }

    params.getPictureSize(&w, &h);

    if ( ( w != ( int ) cap->mWidth ) ||
          ( h != ( int ) cap->mHeight ) )
        {
        mPendingCaptureSettings |= SetFormat;
        }

    cap->mWidth = w;
    cap->mHeight = h;
    //TODO: Support more pixelformats
    //cap->mStride = 2;

    CAMHAL_LOGVB("Image: cap.mWidth = %d", (int)cap->mWidth);
    CAMHAL_LOGVB("Image: cap.mHeight = %d", (int)cap->mHeight);

    mRawCapture = false;

#ifdef CAMERAHAL_USE_RAW_IMAGE_SAVING
    valstr = params.get(TICameraParameters::KEY_CAP_MODE);
    if ( (!valstr || strcmp(valstr, TICameraParameters::HIGH_QUALITY_MODE) == 0) &&
            access(kRawImagesOutputDirPath, F_OK) != -1 ) {
        mRawCapture = true;
    }
#endif

    if ((valstr = params.getPictureFormat()) != NULL) {
        if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV422I;
        } else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV420SP;
        } else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0) {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_RGB565;
        } else if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_JPEG) == 0) {
            CAMHAL_LOGDA("JPEG format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingJPEG;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_JPEG;
        } else if (strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_JPS) == 0) {
            CAMHAL_LOGDA("JPS format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingJPS;
            mPictureFormatFromClient = TICameraParameters::PIXEL_FORMAT_JPS;
        } else if (strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_MPO) == 0) {
            CAMHAL_LOGDA("MPO format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingMPO;
            mPictureFormatFromClient = TICameraParameters::PIXEL_FORMAT_MPO;
        } else if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_BAYER_RGGB) == 0) {
            CAMHAL_LOGDA("RAW Picture format selected");
            pixFormat = OMX_COLOR_FormatRawBayer10bit;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_BAYER_RGGB;
        } else {
            CAMHAL_LOGEA("Invalid format, JPEG format selected as default");
            pixFormat = OMX_COLOR_FormatUnused;
            codingMode = CodingJPEG;
            mPictureFormatFromClient = NULL;
        }
    } else {
        CAMHAL_LOGEA("Picture format is NULL, defaulting to JPEG");
        pixFormat = OMX_COLOR_FormatUnused;
        codingMode = CodingJPEG;
        mPictureFormatFromClient = NULL;
    }

    // JPEG capture is not supported in video mode by OMX Camera
    // Set capture format to yuv422i...jpeg encode will
    // be done on A9
    valstr = params.get(TICameraParameters::KEY_CAP_MODE);
    if ( (valstr && !strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE)) &&
            (pixFormat == OMX_COLOR_FormatUnused) ) {
        CAMHAL_LOGDA("Capturing in video mode...selecting yuv422i");
        pixFormat = OMX_COLOR_FormatCbYCrY;
    }

    if (pixFormat != cap->mColorFormat || codingMode != mCodingMode) {
        mPendingCaptureSettings |= SetFormat;
        cap->mColorFormat = pixFormat;
        mCodingMode = codingMode;
    }

    str = params.get(TICameraParameters::KEY_TEMP_BRACKETING);
    if ( ( str != NULL ) &&
         ( strcmp(str, CameraParameters::TRUE) == 0 ) ) {

        if ( !mBracketingSet ) {
            mPendingCaptureSettings |= SetExpBracket;
        }

        mBracketingSet = true;
    } else {

        if ( mBracketingSet ) {
            mPendingCaptureSettings |= SetExpBracket;
        }

        mBracketingSet = false;
    }

    if ( (str = params.get(TICameraParameters::KEY_EXP_BRACKETING_RANGE)) != NULL ) {
        parseExpRange(str, mExposureBracketingValues, NULL,
                      NULL,
                      EXP_BRACKET_RANGE, mExposureBracketingValidEntries);
        mExposureBracketMode = OMX_BracketExposureRelativeInEV;
        mPendingCaptureSettings |= SetExpBracket;
    } else if ( (str = params.get(TICameraParameters::KEY_EXP_GAIN_BRACKETING_RANGE)) != NULL) {
        parseExpRange(str, mExposureBracketingValues, mExposureGainBracketingValues,
                      mExposureGainBracketingModes,
                      EXP_BRACKET_RANGE, mExposureBracketingValidEntries);
        // TODO(XXX): Use bracket shot for cpcam. Should we let user use exposure
        // bracketing too?
        if (mCapMode == OMXCameraAdapter::CP_CAM) {
            mExposureBracketMode = OMX_BracketVectorShot;
        } else {
            mExposureBracketMode = OMX_BracketExposureGainAbsolute;
        }
        mPendingCaptureSettings |= SetExpBracket;
    } else {
        // if bracketing was previously set...we set again before capturing to clear
        if (mExposureBracketingValidEntries) {
            mPendingCaptureSettings |= SetExpBracket;
            mExposureBracketingValidEntries = 0;
        }
    }

    if ( params.getInt(CameraParameters::KEY_ROTATION) != -1 )
        {
        if (params.getInt(CameraParameters::KEY_ROTATION) != (int) mPictureRotation) {
            mPendingCaptureSettings |= SetRotation;
        }
        mPictureRotation = params.getInt(CameraParameters::KEY_ROTATION);
        }
    else
        {
        if (mPictureRotation) mPendingCaptureSettings |= SetRotation;
        mPictureRotation = 0;
        }

    CAMHAL_LOGVB("Picture Rotation set %d", mPictureRotation);

    // Read Sensor Orientation and set it based on perating mode

    if ( params.getInt(TICameraParameters::KEY_SENSOR_ORIENTATION) != -1 )
        {
        mSensorOrientation = params.getInt(TICameraParameters::KEY_SENSOR_ORIENTATION);
        if (mSensorOrientation == 270 ||mSensorOrientation==90)
            {
            CAMHAL_LOGEA(" Orientation is 270/90. So setting counter rotation to Ducati");
            mSensorOrientation +=180;
            mSensorOrientation%=360;
            }
        }
    else
        {
        mSensorOrientation = 0;
        }

    CAMHAL_LOGVB("Sensor Orientation  set : %d", mSensorOrientation);

    if ( params.getInt(TICameraParameters::KEY_BURST)  >= 1 )
        {
        if (params.getInt(TICameraParameters::KEY_BURST) != (int) mBurstFrames) {
            mPendingCaptureSettings |= SetExpBracket;
        }
        mBurstFrames = params.getInt(TICameraParameters::KEY_BURST);
        }
    else
        {
        if (mBurstFrames != 1) mPendingCaptureSettings |= SetExpBracket;
        mBurstFrames = 1;
        }

    CAMHAL_LOGVB("Burst Frames set %d", mBurstFrames);

    if ( ( params.getInt(CameraParameters::KEY_JPEG_QUALITY)  >= MIN_JPEG_QUALITY ) &&
         ( params.getInt(CameraParameters::KEY_JPEG_QUALITY)  <= MAX_JPEG_QUALITY ) )
        {
        if (params.getInt(CameraParameters::KEY_JPEG_QUALITY) != (int) mPictureQuality) {
            mPendingCaptureSettings |= SetQuality;
        }
        mPictureQuality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
        }
    else
        {
        if (mPictureQuality != MAX_JPEG_QUALITY) mPendingCaptureSettings |= SetQuality;
        mPictureQuality = MAX_JPEG_QUALITY;
        }

    CAMHAL_LOGVB("Picture Quality set %d", mPictureQuality);

    if ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH)  >= 0 )
        {
        if (params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH) != (int) mThumbWidth) {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbWidth = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
        }
    else
        {
        if (mThumbWidth != DEFAULT_THUMB_WIDTH) mPendingCaptureSettings |= SetThumb;
        mThumbWidth = DEFAULT_THUMB_WIDTH;
        }


    CAMHAL_LOGVB("Picture Thumb width set %d", mThumbWidth);

    if ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT)  >= 0 )
        {
        if (params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT) != (int) mThumbHeight) {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbHeight = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
        }
    else
        {
        if (mThumbHeight != DEFAULT_THUMB_HEIGHT) mPendingCaptureSettings |= SetThumb;
        mThumbHeight = DEFAULT_THUMB_HEIGHT;
        }


    CAMHAL_LOGVB("Picture Thumb height set %d", mThumbHeight);

    if ( ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY)  >= MIN_JPEG_QUALITY ) &&
         ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY)  <= MAX_JPEG_QUALITY ) )
        {
        if (params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY) != (int) mThumbQuality) {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbQuality = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
        }
    else
        {
        if (mThumbQuality != MAX_JPEG_QUALITY) mPendingCaptureSettings |= SetThumb;
        mThumbQuality = MAX_JPEG_QUALITY;
        }

    CAMHAL_LOGDB("Thumbnail Quality set %d", mThumbQuality);

    if (mFirstTimeInit) {
        mPendingCaptureSettings = ECapturesettingsAll;
    }

    if (mPendingCaptureSettings) {
        disableImagePort();
        if ( NULL != mReleaseImageBuffersCallback ) {
            mReleaseImageBuffersCallback(mReleaseData);
        }
        if (mPendingCaptureSettings & SetFormat) {
            mPendingCaptureSettings &= ~SetFormat;
            ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *cap);
            if ( ret != NO_ERROR ) {
                CAMHAL_LOGEB("setFormat() failed %d", ret);
                LOG_FUNCTION_NAME_EXIT;
                return ret;
            }
        }
    }

    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex];
    cap->mWidth = params.getInt(TICameraParameters::RAW_WIDTH);
    cap->mHeight = params.getInt(TICameraParameters::RAW_HEIGHT);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *imgCaptureData = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {
        imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

        imgCaptureData->mNumBufs = bufferCount;

        // check if image port is already configured...
        // if it already configured then we don't have to query again
        if (!mCaptureConfigured) {
            ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        }

        if ( ret == NO_ERROR )
            {
            length = imgCaptureData->mBufSize;
            }
        else
            {
            CAMHAL_LOGEB("setFormat() failed 0x%x", ret);
            length = 0;
            }
        }

    CAMHAL_LOGDB("getPictureBufferSize %d", length);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int OMXCameraAdapter::getBracketingValueMode(const char *a, const char *b) const
{
    BracketingValueMode bvm = BracketingValueAbsolute;

    if ( (NULL != b) &&
         (NULL != a) &&
         (a < b) &&
         ( (NULL != memchr(a, '+', b - a)) ||
           (NULL != memchr(a, '-', b - a)) ) ) {
        bvm = BracketingValueRelative;
    }
    return bvm;
}

status_t OMXCameraAdapter::parseExpRange(const char *rangeStr,
                                         int *expRange,
                                         int *gainRange,
                                         int *expGainModes,
                                         size_t count,
                                         size_t &validEntries)
{
    status_t ret = NO_ERROR;
    char *end = NULL;
    const char *startPtr = NULL;
    size_t i = 0;

    LOG_FUNCTION_NAME;

    if ( NULL == rangeStr ){
        return -EINVAL;
    }

    if ( NULL == expRange ){
        return -EINVAL;
    }

    if ( NO_ERROR == ret ) {
        startPtr = rangeStr;
        do {
            // Relative Exposure example: "-30,-10, 0, 10, 30"
            // Absolute Gain ex. (exposure,gain) pairs: "(100,300),(200,300),(400,300),(800,300),(1600,300)"
            // Relative Gain ex. (exposure,gain) pairs: "(-30,+0),(-10, +0),(+0,+0),(+10,+0),(+30,+0)"
            // Forced relative Gain ex. (exposure,gain) pairs: "(-30,+0)F,(-10, +0)F,(+0,+0)F,(+10,+0)F,(+30,+0)F"

            // skip '(' and ','
            while ((*startPtr == '(') ||  (*startPtr == ',')) startPtr++;

            expRange[i] = (int)strtol(startPtr, &end, 10);

            int bvm_exp = getBracketingValueMode(startPtr, end);

            // if gainRange is given rangeStr should be (exposure, gain) pair
            if (gainRange && expGainModes) {
                startPtr = end + 1; // for the ','
                gainRange[i] = (int)strtol(startPtr, &end, 10);

                if (BracketingValueAbsolute == bvm_exp) {
                    expGainModes[i] = getBracketingValueMode(startPtr, end);
                } else {
                    expGainModes[i] = bvm_exp;
                }
            }
            startPtr = end;

            // skip ')'
            while (*startPtr == ')') startPtr++;

            // Check for "forced" key
            if (expGainModes) {
                while ((*startPtr == 'F') || (*startPtr == 'f')) {
                    expGainModes[i] = BracketingValueForcedRelative;
                    startPtr++;
                }
            }

            i++;

        } while ((startPtr[0] != '\0') && (i < count));
        validEntries = i;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::doExposureBracketing(int *evValues,
                                                 int *evValues2,
                                                 int *evModes2,
                                                 size_t evCount,
                                                 size_t frameCount)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState ) {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
    }

    if ( NULL == evValues ) {
        CAMHAL_LOGEA("Exposure compensation values pointer is invalid");
        ret = -EINVAL;
    }

    if ( NO_ERROR == ret ) {
        if (mExposureBracketMode == OMX_BracketVectorShot) {
            ret = setVectorShot(evValues, evValues2, evModes2, evCount, frameCount);
        } else {
            ret = setExposureBracketing(evValues, evValues2, evCount, frameCount);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setVectorStop(bool toPreview)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_VECTSHOTSTOPMETHODTYPE vecShotStop;


    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR(&vecShotStop, OMX_TI_CONFIG_VECTSHOTSTOPMETHODTYPE);

    vecShotStop.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    if (toPreview) {
        vecShotStop.eStopMethod =  OMX_TI_VECTSHOTSTOPMETHOD_GOTO_PREVIEW;
    } else {
        vecShotStop.eStopMethod =  OMX_TI_VECTSHOTSTOPMETHOD_WAIT_IN_CAPTURE;
    }

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE) OMX_TI_IndexConfigVectShotStopMethod,
                           &vecShotStop);
    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGEB("Error while configuring bracket shot 0x%x", eError);
    } else {
        CAMHAL_LOGDA("Bracket shot configured successfully");
    }

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::setVectorShot(int *evValues,
                                         int *evValues2,
                                         int *evModes2,
                                         size_t evCount,
                                         size_t frameCount)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAPTUREMODETYPE expCapMode;
    OMX_CONFIG_EXTCAPTUREMODETYPE extExpCapMode;
    OMX_TI_CONFIG_ENQUEUESHOTCONFIGS enqueueShotConfigs;
    OMX_TI_CONFIG_QUERYAVAILABLESHOTS queryAvailableShots;


    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR(&enqueueShotConfigs, OMX_TI_CONFIG_ENQUEUESHOTCONFIGS);
    OMX_INIT_STRUCT_PTR(&queryAvailableShots, OMX_TI_CONFIG_QUERYAVAILABLESHOTS);

    queryAvailableShots.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                                (OMX_INDEXTYPE) OMX_TI_IndexConfigQueryAvailableShots,
                                &queryAvailableShots);
    if (OMX_ErrorNone != eError) {
        CAMHAL_LOGE("Error getting available shots 0x%x", eError);
        goto exit;
    } else {
        CAMHAL_LOGD("AVAILABLE SHOTS: %d", queryAvailableShots.nAvailableShots);
        if (queryAvailableShots.nAvailableShots < evCount) {
            // TODO(XXX): Need to implement some logic to handle this error
            CAMHAL_LOGE("Not enough available shots to fulfill this queue request");
            ret = -ENOSPC;
            goto exit;
        }
    }

    if (NO_ERROR == ret) {
        OMX_INIT_STRUCT_PTR (&expCapMode, OMX_CONFIG_CAPTUREMODETYPE);
        expCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        expCapMode.bFrameLimited = OMX_FALSE;

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                OMX_IndexConfigCaptureMode,
                                &expCapMode);
        if (OMX_ErrorNone != eError) {
            CAMHAL_LOGEB("Error while configuring capture mode 0x%x", eError);
            goto exit;
        } else {
            CAMHAL_LOGDA("Camera capture mode configured successfully");
        }
    }

    if (NO_ERROR == ret) {
        OMX_INIT_STRUCT_PTR (&extExpCapMode, OMX_CONFIG_EXTCAPTUREMODETYPE);
        extExpCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        if ( 0 == evCount ) {
            extExpCapMode.bEnableBracketing = OMX_FALSE;
        } else {
            extExpCapMode.bEnableBracketing = OMX_TRUE;
            extExpCapMode.tBracketConfigType.eBracketMode = mExposureBracketMode;
        }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigExtCaptureMode,
                                &extExpCapMode);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGEB("Error while configuring extended capture mode 0x%x", eError);
            goto exit;
        } else {
            CAMHAL_LOGDA("Extended camera capture mode configured successfully");
        }
    }

    if (NO_ERROR == ret) {
        // set vector stop method to stop in capture
        ret = setVectorStop(false);
    }

    if ( NO_ERROR == ret )
    {
        unsigned int i;
        for ( i = 0 ; i < evCount ; i++ ) {
                CAMHAL_LOGD("%d: (%d,%d) mode: %d", i, evValues[i], evValues2[i], evModes2[i]);
                enqueueShotConfigs.nShotConfig[i].nConfigId = i;
                enqueueShotConfigs.nShotConfig[i].nFrames = 1;
                enqueueShotConfigs.nShotConfig[i].nExp = evValues[i];
                enqueueShotConfigs.nShotConfig[i].nGain = evValues2[i];
                enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_ABSOLUTE;
                switch (evModes2[i]) {
                    case BracketingValueAbsolute: // (exp,gain) pairs directly program sensor values
                    default :
                    enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_ABSOLUTE;
                        break;
                    case BracketingValueRelative: // (exp,gain) pairs relative to AE settings and constraints
                    enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_RELATIVE;
                        break;
                    case BracketingValueForcedRelative: // (exp, gain) pairs relative to AE settings AND settings
                                                        // are forced over constraints due to flicker, etc.
                    enqueueShotConfigs.nShotConfig[i].eExpGainApplyMethod = OMX_TI_EXPGAINAPPLYMETHOD_FORCE_RELATIVE;
                        break;
                }
                enqueueShotConfigs.nShotConfig[i].bNoSnapshot = OMX_FALSE; // TODO: Make this configurable
        }

        // Repeat last exposure and again
        if ((evCount > 0) && (frameCount > evCount)) {
            enqueueShotConfigs.nShotConfig[i-1].nFrames = frameCount - evCount;
        }

        if (mExposureBracketMode == OMX_BracketVectorShot) {
            enqueueShotConfigs.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
            enqueueShotConfigs.bFlushQueue = OMX_FALSE;
            enqueueShotConfigs.nNumConfigs = evCount;
            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            ( OMX_INDEXTYPE ) OMX_TI_IndexConfigEnqueueShotConfigs,
                                &enqueueShotConfigs);
            if ( OMX_ErrorNone != eError ) {
                CAMHAL_LOGEB("Error while configuring bracket shot 0x%x", eError);
                goto exit;
            } else {
                 CAMHAL_LOGDA("Bracket shot configured successfully");
            }
        }
    }

 exit:
    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::setExposureBracketing(int *evValues,
                                                 int *evValues2,
                                                 size_t evCount,
                                                 size_t frameCount)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAPTUREMODETYPE expCapMode;
    OMX_CONFIG_EXTCAPTUREMODETYPE extExpCapMode;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&expCapMode, OMX_CONFIG_CAPTUREMODETYPE);
        expCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        /// If frameCount>0 but evCount<=0, then this is the case of HQ burst.
        //Otherwise, it is normal HQ capture
        ///If frameCount>0 and evCount>0 then this is the cause of HQ Exposure bracketing.
        if ( 0 == evCount && 0 == frameCount )
            {
            expCapMode.bFrameLimited = OMX_FALSE;
            }
        else
            {
            expCapMode.bFrameLimited = OMX_TRUE;
            expCapMode.nFrameLimit = frameCount;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                OMX_IndexConfigCaptureMode,
                                &expCapMode);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring capture mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Camera capture mode configured successfully");
            }
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&extExpCapMode, OMX_CONFIG_EXTCAPTUREMODETYPE);
        extExpCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        if ( 0 == evCount )
            {
            extExpCapMode.bEnableBracketing = OMX_FALSE;
            }
        else
            {
            extExpCapMode.bEnableBracketing = OMX_TRUE;
            extExpCapMode.tBracketConfigType.eBracketMode = mExposureBracketMode;
            extExpCapMode.tBracketConfigType.nNbrBracketingValues = evCount - 1;
            }

        for ( unsigned int i = 0 ; i < evCount ; i++ )
            {
            if (mExposureBracketMode == OMX_BracketExposureGainAbsolute) {
                extExpCapMode.tBracketConfigType.nBracketValues[i]  =  evValues[i];
                extExpCapMode.tBracketConfigType.nBracketValues2[i]  =  evValues2[i];
            } else {
                // assuming OMX_BracketExposureRelativeInEV
                extExpCapMode.tBracketConfigType.nBracketValues[i]  =  ( evValues[i] * ( 1 << Q16_OFFSET ) )  / 10;
            }
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigExtCaptureMode,
                                &extExpCapMode);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring extended capture mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Extended camera capture mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setShutterCallback(bool enabled)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CALLBACKREQUESTTYPE shutterRequstCallback;

    LOG_FUNCTION_NAME;

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&shutterRequstCallback, OMX_CONFIG_CALLBACKREQUESTTYPE);
        shutterRequstCallback.nPortIndex = OMX_ALL;

        if ( enabled )
            {
            shutterRequstCallback.bEnable = OMX_TRUE;
            shutterRequstCallback.nIndex = ( OMX_INDEXTYPE ) OMX_TI_IndexConfigShutterCallback;
            CAMHAL_LOGDA("Enabling shutter callback");
            }
        else
            {
            shutterRequstCallback.bEnable = OMX_FALSE;
            shutterRequstCallback.nIndex = ( OMX_INDEXTYPE ) OMX_TI_IndexConfigShutterCallback;
            CAMHAL_LOGDA("Disabling shutter callback");
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigCallbackRequest,
                                &shutterRequstCallback);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error registering shutter callback 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDB("Shutter callback for index 0x%x registered successfully",
                         OMX_TI_IndexConfigShutterCallback);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::doBracketing(OMX_BUFFERHEADERTYPE *pBuffHeader,
                                        CameraFrame::FrameType typeOfFrame)
{
    status_t ret = NO_ERROR;
    int currentBufferIdx, nextBufferIdx;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        currentBufferIdx = ( unsigned int ) pBuffHeader->pAppPrivate;

        if ( currentBufferIdx >= imgCaptureData->mNumBufs)
            {
            CAMHAL_LOGEB("Invalid bracketing buffer index 0x%x", currentBufferIdx);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {
        mBracketingBuffersQueued[currentBufferIdx] = false;
        mBracketingBuffersQueuedCount--;

        if ( 0 >= mBracketingBuffersQueuedCount )
            {
            nextBufferIdx = ( currentBufferIdx + 1 ) % imgCaptureData->mNumBufs;
            mBracketingBuffersQueued[nextBufferIdx] = true;
            mBracketingBuffersQueuedCount++;
            mLastBracetingBufferIdx = nextBufferIdx;
            setFrameRefCount(imgCaptureData->mBufferHeader[nextBufferIdx]->pBuffer, typeOfFrame, 1);
            returnFrame(imgCaptureData->mBufferHeader[nextBufferIdx]->pBuffer, typeOfFrame);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::sendBracketFrames(size_t &framesSent)
{
    status_t ret = NO_ERROR;
    int currentBufferIdx;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    framesSent = 0;

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {

        currentBufferIdx = mLastBracetingBufferIdx;
        do
            {
            currentBufferIdx++;
            currentBufferIdx %= imgCaptureData->mNumBufs;
            if (!mBracketingBuffersQueued[currentBufferIdx] )
                {
                CameraFrame cameraFrame;
                sendCallBacks(cameraFrame,
                              imgCaptureData->mBufferHeader[currentBufferIdx],
                              imgCaptureData->mImageType,
                              imgCaptureData);
                framesSent++;
                }
            } while ( currentBufferIdx != mLastBracetingBufferIdx );

        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startBracketing(int range)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

        {
        Mutex::Autolock lock(mBracketingLock);

        if ( mBracketingEnabled )
            {
            return ret;
            }
        }

    if ( 0 == imgCaptureData->mNumBufs )
        {
        CAMHAL_LOGEB("Image capture buffers set to %d", imgCaptureData->mNumBufs);
        ret = -EINVAL;
        }

    if ( mPending3Asettings )
        apply3Asettings(mParameters3A);

    if ( NO_ERROR == ret )
        {
        Mutex::Autolock lock(mBracketingLock);

        mBracketingRange = range;
        mBracketingBuffersQueued = new bool[imgCaptureData->mNumBufs];
        if ( NULL == mBracketingBuffersQueued )
            {
            CAMHAL_LOGEA("Unable to allocate bracketing management structures");
            ret = -1;
            }

        if ( NO_ERROR == ret )
            {
            mBracketingBuffersQueuedCount = imgCaptureData->mNumBufs;
            mLastBracetingBufferIdx = mBracketingBuffersQueuedCount - 1;

            for ( int i = 0 ; i  < imgCaptureData->mNumBufs ; i++ )
                {
                mBracketingBuffersQueued[i] = true;
                }

            }
        }

    if ( NO_ERROR == ret )
        {

        ret = startImageCapture(true);
            {
            Mutex::Autolock lock(mBracketingLock);

            if ( NO_ERROR == ret )
                {
                mBracketingEnabled = true;
                }
            else
                {
                mBracketingEnabled = false;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::stopBracketing()
{
  status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mBracketingLock);

    if ( NULL != mBracketingBuffersQueued )
    {
        delete [] mBracketingBuffersQueued;
    }

    ret = stopImageCapture();

    mBracketingBuffersQueued = NULL;
    mBracketingEnabled = false;
    mBracketingBuffersQueuedCount = 0;
    mLastBracetingBufferIdx = 0;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startImageCapture(bool bracketing)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * capData = NULL;
    OMX_CONFIG_BOOLEANTYPE bOMX;
    size_t bracketingSent = 0;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mImageCaptureLock);

    if(!mCaptureConfigured)
        {
        ///Image capture was cancelled before we could start
        return NO_ERROR;
        }

    if ( 0 != mStartCaptureSem.Count() )
        {
        CAMHAL_LOGEB("Error mStartCaptureSem semaphore count %d", mStartCaptureSem.Count());
        return NO_INIT;
        }

    if ( !bracketing ) {
        if ((getNextState() & (CAPTURE_ACTIVE|BRACKETING_ACTIVE)) == 0) {
            CAMHAL_LOGDA("trying starting capture when already canceled");
            return NO_ERROR;
        }
    }

    // Camera framework doesn't expect face callbacks once capture is triggered
    pauseFaceDetection(true);

    //During bracketing image capture is already active
    {
    Mutex::Autolock lock(mBracketingLock);
    if ( mBracketingEnabled )
        {
        //Stop bracketing, activate normal burst for the remaining images
        mBracketingEnabled = false;
        ret = sendBracketFrames(bracketingSent);

        // Check if we accumulated enough buffers
        if ( bracketingSent < ( mBracketingRange - 1 ) )
            {
            mCapturedFrames = mBracketingRange + ( ( mBracketingRange - 1 ) - bracketingSent );
            }
        else
            {
            mCapturedFrames = mBracketingRange;
            }

        if(ret != NO_ERROR)
            goto EXIT;
        else
            return ret;
        }
    }

    if ( NO_ERROR == ret ) {
        if (mPendingCaptureSettings & SetRotation) {
            mPendingCaptureSettings &= ~SetRotation;
            ret = setPictureRotation(mPictureRotation);
            if ( NO_ERROR != ret ) {
                CAMHAL_LOGEB("Error configuring image rotation %x", ret);
            }
        }

        if (mPendingCaptureSettings & SetExpBracket) {
            mPendingCaptureSettings &= ~SetExpBracket;
            if ( mBracketingSet ) {
                ret = doExposureBracketing(mExposureBracketingValues,
                                            mExposureGainBracketingValues,
                                            mExposureGainBracketingModes,
                                            0,
                                            0);
            } else {
                ret = doExposureBracketing(mExposureBracketingValues,
                                    mExposureGainBracketingValues,
                                    mExposureGainBracketingModes,
                                    mExposureBracketingValidEntries,
                                    mBurstFrames);
            }

            if ( ret != NO_ERROR ) {
                CAMHAL_LOGEB("setExposureBracketing() failed %d", ret);
                goto EXIT;
            }
        }
    }

    // need to enable wb data for video snapshot to fill in exif data
    if ((ret == NO_ERROR) && (mCapMode == VIDEO_MODE)) {
        // video snapshot uses wb data from snapshot frame
        ret = setExtraData(true, mCameraAdapterParameters.mPrevPortIndex, OMX_WhiteBalance);
    }

    //OMX shutter callback events are only available in hq mode
    if ( (HIGH_QUALITY == mCapMode) || (HIGH_QUALITY_ZSL== mCapMode))
        {

        if ( NO_ERROR == ret )
            {
            ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                        (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                                        OMX_ALL,
                                        OMX_TI_IndexConfigShutterCallback,
                                        mStartCaptureSem);
            }

        if ( NO_ERROR == ret )
            {
            ret = setShutterCallback(true);
            }

        }

    if (mPending3Asettings) {
        apply3Asettings(mParameters3A);
    }

    if ( NO_ERROR == ret ) {
        capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

        ///Queue all the buffers on capture port
        for ( int index = 0 ; index < capData->mNumBufs ; index++ ) {
            CAMHAL_LOGDB("Queuing buffer on Capture port - 0x%x",
                         ( unsigned int ) capData->mBufferHeader[index]->pBuffer);
            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                        (OMX_BUFFERHEADERTYPE*)capData->mBufferHeader[index]);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

        if (mRawCapture) {
            capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex];

            ///Queue all the buffers on capture port
            for ( int index = 0 ; index < capData->mNumBufs ; index++ ) {
                CAMHAL_LOGDB("Queuing buffer on Video port (for RAW capture) - 0x%x", ( unsigned int ) capData->mBufferHeader[index]->pBuffer);
                eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                        (OMX_BUFFERHEADERTYPE*)capData->mBufferHeader[index]);

                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }
        }
        mWaitingForSnapshot = true;
        mCaptureSignalled = false;

        // Capturing command is not needed when capturing in video mode
        // Only need to queue buffers on image ports
        if (mCapMode != VIDEO_MODE) {
            OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);
            bOMX.bEnabled = OMX_TRUE;

            /// sending Capturing Command to the component
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                   OMX_IndexConfigCapturing,
                                   &bOMX);

            CAMHAL_LOGDB("Capture set - 0x%x", eError);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }
    }

    //OMX shutter callback events are only available in hq mode
    if ( (HIGH_QUALITY == mCapMode) || (HIGH_QUALITY_ZSL== mCapMode))
        {

        if ( NO_ERROR == ret )
            {
            ret = mStartCaptureSem.WaitTimeout(OMX_CAPTURE_TIMEOUT);
            }

        //If something bad happened while we wait
        if (mComponentState != OMX_StateExecuting)
          {
            CAMHAL_LOGEA("Invalid State after Image Capture Exitting!!!");
            goto EXIT;
          }

        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Shutter callback received");
            notifyShutterSubscribers();
            }
        else
            {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                               OMX_ALL,
                               OMX_TI_IndexConfigShutterCallback,
                               NULL);
            CAMHAL_LOGEA("Timeout expired on shutter callback");
            goto EXIT;
            }

        }

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    setExtraData(false, mCameraAdapterParameters.mPrevPortIndex, OMX_WhiteBalance);
    mWaitingForSnapshot = false;
    mCaptureSignalled = false;
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE bOMX;
    OMXCameraPortParameters *imgCaptureData = NULL;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mImageCaptureLock);

    if (!mCaptureConfigured) {
        //Capture is not ongoing, return from here
        return NO_ERROR;
    }

    if ( 0 != mStopCaptureSem.Count() ) {
        CAMHAL_LOGEB("Error mStopCaptureSem semaphore count %d", mStopCaptureSem.Count());
        goto EXIT;
    }

    //Disable the callback first
    mWaitingForSnapshot = false;
    mSnapshotCount = 0;

    // OMX shutter callback events are only available in hq mode
    if ((HIGH_QUALITY == mCapMode) || (HIGH_QUALITY_ZSL== mCapMode)) {
        //Disable the callback first
        ret = setShutterCallback(false);

        // if anybody is waiting on the shutter callback
        // signal them and then recreate the semaphore
        if ( 0 != mStartCaptureSem.Count() ) {

            for (int i = mStartCaptureSem.Count(); i < 0; i++) {
            ret |= SignalEvent(mCameraAdapterParameters.mHandleComp,
                               (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                               OMX_ALL,
                               OMX_TI_IndexConfigShutterCallback,
                               NULL );
            }
            mStartCaptureSem.Create(0);
        }
    }

    // After capture, face detection should be disabled
    // and application needs to restart face detection
    stopFaceDetection();

    //Wait here for the capture to be done, in worst case timeout and proceed with cleanup
    mCaptureSem.WaitTimeout(OMX_CAPTURE_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State Image Capture Stop Exitting!!!");
        goto EXIT;
      }

    // Disable image capture
    // Capturing command is not needed when capturing in video mode
    if (mCapMode != VIDEO_MODE) {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);
        bOMX.bEnabled = OMX_FALSE;
        imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               OMX_IndexConfigCapturing,
                               &bOMX);
        if ( OMX_ErrorNone != eError ) {
            CAMHAL_LOGDB("Error during SetConfig- 0x%x", eError);
            ret = -1;
            goto EXIT;
        }
    }

    // had to enable wb data for video snapshot to fill in exif data
    // now that we are done...disable
    if ((ret == NO_ERROR) && (mCapMode == VIDEO_MODE)) {
        ret = setExtraData(false, mCameraAdapterParameters.mPrevPortIndex, OMX_WhiteBalance);
    }

    CAMHAL_LOGDB("Capture set - 0x%x", eError);

    mCaptureSignalled = true; //set this to true if we exited because of timeout

    {
        Mutex::Autolock lock(mFrameCountMutex);
        mFrameCount = 0;
        mFirstFrameCondition.broadcast();
    }

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }

    {
        Mutex::Autolock lock(mFrameCountMutex);
        mFrameCount = 0;
        mFirstFrameCondition.broadcast();
    }

    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::disableImagePort(){
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *imgCaptureData = NULL;
    OMXCameraPortParameters *imgRawCaptureData = NULL;

    if (!mCaptureConfigured) {
        return NO_ERROR;
    }

    mCaptureConfigured = false;
    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    imgRawCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex]; // for RAW capture

    ///Register for Image port Disable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mImagePortIndex,
                                mStopCaptureSem);
    ///Disable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mImagePortIndex,
                                NULL);

    ///Free all the buffers on capture port
    if (imgCaptureData) {
        CAMHAL_LOGDB("Freeing buffer on Capture port - %d", imgCaptureData->mNumBufs);
        for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++) {
            CAMHAL_LOGDB("Freeing buffer on Capture port - 0x%x",
                         ( unsigned int ) imgCaptureData->mBufferHeader[index]->pBuffer);
            eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                    mCameraAdapterParameters.mImagePortIndex,
                                    (OMX_BUFFERHEADERTYPE*)imgCaptureData->mBufferHeader[index]);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }
    }
    CAMHAL_LOGDA("Waiting for port disable");
    //Wait for the image port enable event
    ret = mStopCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Disable Image Port Exitting!!!");
        goto EXIT;
      }

    if ( NO_ERROR == ret ) {
        CAMHAL_LOGDA("Port disabled");
    } else {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mImagePortIndex,
                           NULL);
        CAMHAL_LOGDA("Timeout expired on port disable");
        goto EXIT;
    }

    if (mRawCapture) {

        ///Register for Video port Disable event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                OMX_EventCmdComplete,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                mStopCaptureSem);
        ///Disable RawCapture Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                NULL);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        ///Free all the buffers on RawCapture port
        if (imgRawCaptureData) {
            CAMHAL_LOGDB("Freeing buffer on Capture port - %d", imgRawCaptureData->mNumBufs);
            for ( int index = 0 ; index < imgRawCaptureData->mNumBufs ; index++) {
                CAMHAL_LOGDB("Freeing buffer on Capture port - 0x%x", ( unsigned int ) imgRawCaptureData->mBufferHeader[index]->pBuffer);
                eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                        mCameraAdapterParameters.mVideoPortIndex,
                        (OMX_BUFFERHEADERTYPE*)imgRawCaptureData->mBufferHeader[index]);

                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }
        }
        CAMHAL_LOGDA("Waiting for Video port disable");
        //Wait for the image port enable event
        mStopCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);
        CAMHAL_LOGDA("Video Port disabled");
    }

EXIT:
    return (ret | ErrorUtils::omxToAndroidError(eError));
}


status_t OMXCameraAdapter::UseBuffersCapture(void* bufArr, int num)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * imgCaptureData = NULL;
    uint32_t *buffers = (uint32_t*)bufArr;
    OMXCameraPortParameters cap;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( 0 != mUseCaptureSem.Count() )
        {
        CAMHAL_LOGEB("Error mUseCaptureSem semaphore count %d", mUseCaptureSem.Count());
        return BAD_VALUE;
        }

    // capture is already configured...we can skip this step
    if (mCaptureConfigured) {

        if ( NO_ERROR == ret )
            {
            ret = setupEXIF();
            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
                }
            }

        mCapturedFrames = mBurstFrames;
        return NO_ERROR;
    }

    imgCaptureData->mNumBufs = num;

    //TODO: Support more pixelformats

    CAMHAL_LOGDB("Params Width = %d", (int)imgCaptureData->mWidth);
    CAMHAL_LOGDB("Params Height = %d", (int)imgCaptureData->mHeight);

    if (mPendingCaptureSettings & SetFormat) {
        mPendingCaptureSettings &= ~SetFormat;
        ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        if ( ret != NO_ERROR ) {
            CAMHAL_LOGEB("setFormat() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            return ret;
        }
    }

    if (mPendingCaptureSettings & SetThumb) {
        mPendingCaptureSettings &= ~SetThumb;
        ret = setThumbnailParams(mThumbWidth, mThumbHeight, mThumbQuality);
        if ( NO_ERROR != ret) {
            CAMHAL_LOGEB("Error configuring thumbnail size %x", ret);
            return ret;
        }
    }

    if (mPendingCaptureSettings & SetQuality) {
        mPendingCaptureSettings &= ~SetQuality;
        ret = setImageQuality(mPictureQuality);
        if ( NO_ERROR != ret) {
            CAMHAL_LOGEB("Error configuring image quality %x", ret);
            goto EXIT;
        }
    }

    ///Register for Image port ENABLE event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mImagePortIndex,
                           mUseCaptureSem);

    ///Enable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortEnable,
                             mCameraAdapterParameters.mImagePortIndex,
                             NULL);

    CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
    GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

    for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++ )
    {
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        CAMHAL_LOGDB("OMX_UseBuffer Capture address: 0x%x, size = %d",
                     (unsigned int)buffers[index],
                     (int)imgCaptureData->mBufSize);

        eError = OMX_UseBuffer(mCameraAdapterParameters.mHandleComp,
                               &pBufferHdr,
                               mCameraAdapterParameters.mImagePortIndex,
                               0,
                               imgCaptureData->mBufSize,
                               (OMX_U8*)buffers[index]);

        CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        pBufferHdr->pAppPrivate = (OMX_PTR) index;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0;
        pBufferHdr->nVersion.s.nStep =  0;
        imgCaptureData->mBufferHeader[index] = pBufferHdr;
    }

    //Wait for the image port enable event
    CAMHAL_LOGDA("Waiting for port enable");
    ret = mUseCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == OMX_StateInvalid)
      {
        CAMHAL_LOGEA("Invalid State after Enable Image Port Exitting!!!");
        goto EXIT;
      }

    if ( ret == NO_ERROR )
        {
        CAMHAL_LOGDA("Port enabled");
        }
    else
        {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mImagePortIndex,
                           NULL);
        CAMHAL_LOGDA("Timeout expired on port enable");
        goto EXIT;
        }

    if ( NO_ERROR == ret )
        {
        ret = setupEXIF();
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
            }
        }

    mCapturedFrames = mBurstFrames;

    if (!mRawCapture) {
        mCaptureConfigured = true;
    }

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback ) {
        mReleaseImageBuffersCallback(mReleaseData);
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));

}
status_t OMXCameraAdapter::UseBuffersRawCapture(void* bufArr, int num)
{
    LOG_FUNCTION_NAME
    status_t ret;
    OMX_ERRORTYPE eError;
    OMXCameraPortParameters * imgRawCaptureData = NULL;
    uint32_t *buffers = (uint32_t*)bufArr;
    Semaphore camSem;
    OMXCameraPortParameters cap;

    imgRawCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mVideoPortIndex];

    if (mCaptureConfigured) {
        return NO_ERROR;
    }

    camSem.Create();

    // mWaitingForSnapshot is true only when we're in the process of capturing
    if (mWaitingForSnapshot) {
        ///Register for Video port Disable event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                (OMX_EVENTTYPE) OMX_EventCmdComplete,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                camSem);

        ///Disable Capture Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                OMX_CommandPortDisable,
                mCameraAdapterParameters.mVideoPortIndex,
                NULL);

        CAMHAL_LOGDA("Waiting for port disable");
        //Wait for the image port enable event
        camSem.Wait();
        CAMHAL_LOGDA("Port disabled");
    }

    imgRawCaptureData->mNumBufs = num;

    CAMHAL_LOGDB("RAW Max sensor width = %d", (int)imgRawCaptureData->mWidth);
    CAMHAL_LOGDB("RAW Max sensor height = %d", (int)imgRawCaptureData->mHeight);

    ret = setFormat(OMX_CAMERA_PORT_VIDEO_OUT_VIDEO, *imgRawCaptureData);

    if (ret != NO_ERROR) {
        CAMHAL_LOGEB("setFormat() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }

    ///Register for Video port ENABLE event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                (OMX_EVENTTYPE) OMX_EventCmdComplete,
                                OMX_CommandPortEnable,
                                mCameraAdapterParameters.mVideoPortIndex,
                                camSem);

    ///Enable Video Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandPortEnable,
                                mCameraAdapterParameters.mVideoPortIndex,
                                NULL);

    mCaptureBuffersLength = (int)imgRawCaptureData->mBufSize;
    for ( int index = 0 ; index < imgRawCaptureData->mNumBufs ; index++ ) {
        OMX_BUFFERHEADERTYPE *pBufferHdr;

        eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                &pBufferHdr,
                                mCameraAdapterParameters.mVideoPortIndex,
                                0,
                                mCaptureBuffersLength,
                                (OMX_U8*)buffers[index]);
        if (eError != OMX_ErrorNone) {
            CAMHAL_LOGEB("OMX_UseBuffer = 0x%x", eError);
        }

        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        pBufferHdr->pAppPrivate = (OMX_PTR) index;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0;
        pBufferHdr->nVersion.s.nStep =  0;
        imgRawCaptureData->mBufferHeader[index] = pBufferHdr;

    }

    //Wait for the image port enable event
    CAMHAL_LOGDA("Waiting for port enable");
    camSem.Wait();
    CAMHAL_LOGDA("Port enabled");

    if (NO_ERROR == ret) {
        ret = setupEXIF();
        if ( NO_ERROR != ret ) {
            CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
        }
    }

    mCapturedFrames = mBurstFrames;
    mCaptureConfigured = true;

    EXIT:

    if (eError != OMX_ErrorNone) {
        if ( NULL != mErrorNotifier )
        {
            mErrorNotifier->errorNotify(eError);
        }
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

};
