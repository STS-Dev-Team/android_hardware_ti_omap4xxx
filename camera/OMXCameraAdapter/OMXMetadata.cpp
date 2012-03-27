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
* @file OMX3A.cpp
*
* This file contains functionality for handling 3A configurations.
*
*/

#undef LOG_TAG

#define LOG_TAG "OMXMetaData"

#include "OMXCameraAdapter.h"

namespace android {

status_t OMXCameraAdapter::setMetaData(CameraMetadata &meta_data, const OMX_TI_PLATFORMPRIVATE * plat_pvt) const
{
    status_t ret = NO_ERROR;
    OMX_OTHER_EXTRADATATYPE *extraData;

    extraData = getExtradata((OMX_OTHER_EXTRADATATYPE*) plat_pvt->pMetaDataBuffer,
                             plat_pvt->nMetaDataSize,
                             (OMX_EXTRADATATYPE) OMX_WhiteBalance);

    if ( NULL != extraData ) {
        OMX_TI_WHITEBALANCERESULTTYPE * WBdata;
        WBdata = (OMX_TI_WHITEBALANCERESULTTYPE*) extraData->data;

        meta_data.set(CameraMetadata::KEY_AWB_TEMP, (int)WBdata->nColorTemperature);
        meta_data.set4(CameraMetadata::KEY_AWB_GAINS,
                                   (int)WBdata->nGainR,
                                   (int)WBdata->nGainGR,
                                   (int)WBdata->nGainGB,
                                   (int)WBdata->nGainB);
        meta_data.set4(CameraMetadata::KEY_AWB_OFFSETS,
                                   (int)WBdata->nOffsetR,
                                   (int)WBdata->nOffsetGR,
                                   (int)WBdata->nOffsetGB,
                                   (int)WBdata->nOffsetB);
    }

    // TODO(XXX): temporarily getting exposure and gain data from vector shot extra data
    // change to ancil or cpcam metadata once Ducati side is ready
    extraData = getExtradata((OMX_OTHER_EXTRADATATYPE*) plat_pvt->pMetaDataBuffer,
                             plat_pvt->nMetaDataSize,
                             (OMX_EXTRADATATYPE) OMX_TI_VectShotInfo);

    if ( NULL != extraData ) {
        OMX_TI_VECTSHOTINFOTYPE *shotInfo;
        shotInfo = (OMX_TI_VECTSHOTINFOTYPE*) extraData->data;

        meta_data.set(CameraMetadata::KEY_ANALOG_GAIN, (int)shotInfo->nAGain);
        meta_data.set(CameraMetadata::KEY_EXPOSURE_TIME, (int)shotInfo->nExpTime);
    }

    return ret;
}

};

