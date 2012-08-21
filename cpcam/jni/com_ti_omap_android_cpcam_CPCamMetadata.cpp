/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <gui/SurfaceTexture.h>
#include <camera/CameraMetadata.h>

#include <binder/IMemory.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

#ifdef ANDROID_API_JB_OR_LATER
#include <gui/BufferQueue.h>
#   define CAMHAL_LOGV ALOGV
#   define CAMHAL_LOGE ALOGE
#   define METADATA_ACCESS_TYPE BufferQueue
#else
#   define CAMHAL_LOGV LOGV
#   define CAMHAL_LOGE LOGE
#   define METADATA_ACCESS_TYPE SurfaceTexture
#endif

const char* const kMetadataAccessClassPathName = "com/ti/omap/android/cpcam/CPCamBufferQueue";
const char* const kMetadataAccessJNIID = "mBufferQueue";

using namespace android;

const char* const kMetadataClassPathName = "com/ti/omap/android/cpcam/CPCamMetadata";

struct fields_t {
    jmethodID metadataInit;
    jmethodID rectInit;
    jmethodID bscPositionInit;
    jmethodID h3aInit;
    jfieldID frameNumber;
    jfieldID shotNumber;
    jfieldID analogGain;
    jfieldID analogGainReq;
    jfieldID analogGainMin;
    jfieldID analogGainMax;
    jfieldID analogGainDev;
    jfieldID analogGainError;
    jfieldID exposureTime;
    jfieldID exposureTimeReq;
    jfieldID exposureTimeMin;
    jfieldID exposureTimeMax;
    jfieldID exposureTimeDev;
    jfieldID exposureTimeError;
    jfieldID exposureCompensationReq;
    jfieldID exposureDev;
    jfieldID timestamp;
    jfieldID awbTemp;
    jfieldID gainR;
    jfieldID gainGR;
    jfieldID gainGB;
    jfieldID gainB;
    jfieldID offsetR;
    jfieldID offsetGR;
    jfieldID offsetGB;
    jfieldID offsetB;
    jfieldID lscTableApplied;
    jfieldID faces;
    jfieldID numberOfFaces;
    jfieldID auxImageWidth;
    jfieldID auxImageHeight;
    jfieldID bscColorElement;
    jfieldID bscRowPosition;
    jfieldID bscColPosition;
    jfieldID afBayeRGBPosition;
    jfieldID afEnableAFPeakMode;
    jfieldID afEnableAFVertical;
    jfieldID afPaxelWindow;
    jfieldID aewbMode;
    jfieldID aewbThresholdPixelValue;
    jfieldID aewbPaxelWindow;
    jfieldID aewbAccumulationShift;
    jfieldID lscTable;
    jfieldID auxImage;
    jfieldID bscRowSum;
    jfieldID bscColSum;
    jfieldID afPaxelStatistics;
    jfieldID aewbPaxelStatistics;
    jfieldID bscPositionVectors;
    jfieldID bscPositionShift;
    jfieldID bscPositionVerticalPosition;
    jfieldID bscPositionHorizontalPosition;
    jfieldID bscPositionVerticalNumber;
    jfieldID bscPositionHorizontalNumber;
    jfieldID bscPositionVerticalSkip;
    jfieldID bscPositionHorizontalSkip;
    jfieldID h3aVerticalPosition;
    jfieldID h3aVerticalSize;
    jfieldID h3aHorizontalPosition;
    jfieldID h3aHorizontalSize;
    jfieldID h3aVerticalCount;
    jfieldID h3aVeticalIncrement;
    jfieldID h3aHorizontalCount;
    jfieldID h3aHorizontalIncrement;

};

static fields_t fields;

static void Metadata_Init(JNIEnv* env, jclass clazz)
{

    jclass metaDataClazz = env->FindClass("com/ti/omap/android/cpcam/CPCamMetadata");
    if ( NULL == metaDataClazz ) {
        CAMHAL_LOGE("Couldn't find CPCamMetadata class");
    }

    fields.metadataInit = env->GetMethodID(metaDataClazz, "<init>", "()V");
    if ( NULL == fields.metadataInit ) {
        CAMHAL_LOGE("Couldn't find Metadata constructor");
    }

    jclass rectClazz = env->FindClass("android/graphics/Rect");
    if ( NULL == rectClazz ) {
        CAMHAL_LOGE("Couldn't find Rect class");
    }

    fields.rectInit = env->GetMethodID(rectClazz, "<init>", "(IIII)V");
    if ( NULL == fields.rectInit ) {
        CAMHAL_LOGE("Couldn't find Rect constructor");
    }

    fields.frameNumber = env->GetFieldID(metaDataClazz, "frameNumber", "I");
    if ( NULL == fields.frameNumber ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.shotNumber = env->GetFieldID(metaDataClazz, "shotNumber", "I");
    if ( NULL == fields.shotNumber ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.analogGain = env->GetFieldID(metaDataClazz, "analogGain", "I");
    if ( NULL == fields.analogGain ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.analogGainReq = env->GetFieldID(metaDataClazz, "analogGainReq", "I");
    if ( NULL == fields.analogGainReq ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.analogGainMin = env->GetFieldID(metaDataClazz, "analogGainMin", "I");
    if ( NULL == fields.analogGainMin ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.analogGainMax = env->GetFieldID(metaDataClazz, "analogGainMax", "I");
    if ( NULL == fields.analogGainMax ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.analogGainDev = env->GetFieldID(metaDataClazz, "analogGainDev", "I");
    if ( NULL == fields.analogGainDev ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.analogGainError = env->GetFieldID(metaDataClazz, "analogGainError", "I");
    if ( NULL == fields.analogGainError ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureTime = env->GetFieldID(metaDataClazz, "exposureTime", "I");
    if ( NULL == fields.exposureTime ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureTimeReq = env->GetFieldID(metaDataClazz, "exposureTimeReq", "I");
    if ( NULL == fields.exposureTimeReq ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureTimeMin = env->GetFieldID(metaDataClazz, "exposureTimeMin", "I");
    if ( NULL == fields.exposureTimeMin ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureTimeMax = env->GetFieldID(metaDataClazz, "exposureTimeMax", "I");
    if ( NULL == fields.exposureTimeMax ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureTimeDev = env->GetFieldID(metaDataClazz, "exposureTimeDev", "I");
    if ( NULL == fields.exposureTimeDev ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureTimeError = env->GetFieldID(metaDataClazz, "exposureTimeError", "I");
    if ( NULL == fields.exposureTimeError ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureCompensationReq = env->GetFieldID(metaDataClazz, "exposureCompensationReq", "I");
    if ( NULL == fields.exposureCompensationReq ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.exposureDev = env->GetFieldID(metaDataClazz, "exposureDev", "I");
    if ( NULL == fields.exposureDev ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.timestamp = env->GetFieldID(metaDataClazz, "timestamp", "J");
    if ( NULL == fields.timestamp ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.awbTemp = env->GetFieldID(metaDataClazz, "awbTemp", "I");
    if ( NULL == fields.awbTemp ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.gainR = env->GetFieldID(metaDataClazz, "gainR", "I");
    if ( NULL == fields.gainR ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.gainGR = env->GetFieldID(metaDataClazz, "gainGR", "I");
    if ( NULL == fields.gainGR ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.gainGB = env->GetFieldID(metaDataClazz, "gainGB", "I");
    if ( NULL == fields.gainGB ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.gainB = env->GetFieldID(metaDataClazz, "gainB", "I");
    if ( NULL == fields.gainB ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.offsetR = env->GetFieldID(metaDataClazz, "offsetR", "I");
    if ( NULL == fields.offsetR ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.offsetGR = env->GetFieldID(metaDataClazz, "offsetGR", "I");
    if ( NULL == fields.offsetGR ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.offsetGB = env->GetFieldID(metaDataClazz, "offsetGB", "I");
    if ( NULL == fields.offsetGB ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.offsetB = env->GetFieldID(metaDataClazz, "offsetB", "I");
    if ( NULL == fields.offsetB ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.lscTableApplied = env->GetFieldID(metaDataClazz, "lscTableApplied", "I");
    if ( NULL == fields.lscTableApplied ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.faces = env->GetFieldID(metaDataClazz, "faces", "Ljava/nio/ByteBuffer;");
    if ( NULL == fields.faces ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.numberOfFaces = env->GetFieldID(metaDataClazz, "numberOfFaces", "I");
    if ( NULL == fields.numberOfFaces ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.auxImageWidth = env->GetFieldID(metaDataClazz, "auxImageWidth", "I");
    if ( NULL == fields.auxImageWidth ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.auxImageHeight = env->GetFieldID(metaDataClazz, "auxImageHeight", "I");
    if ( NULL == fields.auxImageHeight ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.bscColorElement = env->GetFieldID(metaDataClazz, "bscColorElement", "I");
    if ( NULL == fields.bscColorElement ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    jclass bscPositionClazz = env->FindClass("com/ti/omap/android/cpcam/CPCamMetadata$BSCPosition");
    if ( NULL == bscPositionClazz ) {
        CAMHAL_LOGE("Couldn't find BSCPosition class");
    }

    fields.bscPositionInit = env->GetMethodID(bscPositionClazz, "<init>", "()V");
    if ( NULL == fields.bscPositionInit ) {
        CAMHAL_LOGE("Couldn't find BSCPosition constructor");
    }

    fields.bscPositionVectors = env->GetFieldID(bscPositionClazz, "vectors", "I");
    if ( NULL == fields.bscPositionVectors ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscPositionShift = env->GetFieldID(bscPositionClazz, "shift", "I");
    if ( NULL == fields.bscPositionShift ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscPositionVerticalPosition = env->GetFieldID(bscPositionClazz,
                                                         "verticalPosition",
                                                         "I");
    if ( NULL == fields.bscPositionVerticalPosition ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscPositionHorizontalPosition = env->GetFieldID(bscPositionClazz,
                                                           "horizontalPosition",
                                                           "I");
    if ( NULL == fields.bscPositionHorizontalPosition ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscPositionVerticalNumber = env->GetFieldID(bscPositionClazz,
                                                       "verticalNumber",
                                                       "I");
    if ( NULL == fields.bscPositionVerticalNumber ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscPositionHorizontalNumber = env->GetFieldID(bscPositionClazz,
                                                         "horizontalNumber",
                                                         "I");
    if ( NULL == fields.bscPositionHorizontalNumber ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscPositionVerticalSkip = env->GetFieldID(bscPositionClazz,
                                                     "verticalSkip",
                                                     "I");
    if ( NULL == fields.bscPositionVerticalSkip ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscPositionHorizontalSkip = env->GetFieldID(bscPositionClazz,
                                                       "horizontalSkip",
                                                       "I");
    if ( NULL == fields.bscPositionHorizontalSkip ) {
        CAMHAL_LOGE("Couldn't allocate BSCPosition field");
    }

    fields.bscRowPosition = env->GetFieldID(metaDataClazz,
                                            "bscRowPosition",
                                            "Lcom/ti/omap/android/cpcam/CPCamMetadata$BSCPosition;");
    if ( NULL == fields.bscRowPosition ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.bscColPosition = env->GetFieldID(metaDataClazz,
                                            "bscColPosition",
                                            "Lcom/ti/omap/android/cpcam/CPCamMetadata$BSCPosition;");
    if ( NULL == fields.bscColPosition ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.afBayeRGBPosition = env->GetFieldID(metaDataClazz, "afBayeRGBPosition", "I");
    if ( NULL == fields.afBayeRGBPosition ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.afEnableAFPeakMode = env->GetFieldID(metaDataClazz, "afEnableAFPeakMode", "I");
    if ( NULL == fields.afEnableAFPeakMode ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.afEnableAFVertical = env->GetFieldID(metaDataClazz, "afEnableAFVertical", "I");
    if ( NULL == fields.afEnableAFVertical ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.afPaxelWindow = env->GetFieldID(metaDataClazz,
                                                "afPaxelWindow",
                                                "Lcom/ti/omap/android/cpcam/CPCamMetadata$H3AConfig;");
    if ( NULL == fields.afPaxelWindow ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    jclass h3aConfigClazz = env->FindClass("com/ti/omap/android/cpcam/CPCamMetadata$H3AConfig");
    if ( NULL == h3aConfigClazz ) {
        CAMHAL_LOGE("Couldn't find H3AConfig class");
    }

    fields.h3aVerticalPosition = env->GetFieldID(h3aConfigClazz, "verticalPosition", "I");
    if ( NULL == fields.h3aVerticalPosition ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aVerticalSize = env->GetFieldID(h3aConfigClazz, "verticalSize", "I");
    if ( NULL == fields.h3aVerticalSize ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aHorizontalPosition = env->GetFieldID(h3aConfigClazz, "horizontalPosition", "I");
    if ( NULL == fields.h3aHorizontalPosition ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aHorizontalSize = env->GetFieldID(h3aConfigClazz, "horizontalSize", "I");
    if ( NULL == fields.h3aHorizontalSize ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aVerticalCount = env->GetFieldID(h3aConfigClazz, "verticalCount", "I");
    if ( NULL == fields.h3aVerticalCount ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aVeticalIncrement = env->GetFieldID(h3aConfigClazz, "veticalIncrement", "I");
    if ( NULL == fields.h3aVeticalIncrement ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aHorizontalCount = env->GetFieldID(h3aConfigClazz, "horizontalCount", "I");
    if ( NULL == fields.h3aHorizontalCount ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aHorizontalIncrement = env->GetFieldID(h3aConfigClazz, "horizontalIncrement", "I");
    if ( NULL == fields.h3aHorizontalIncrement ) {
        CAMHAL_LOGE("Couldn't allocate H3AConfig field");
    }

    fields.h3aInit = env->GetMethodID(h3aConfigClazz, "<init>", "()V");
    if ( NULL == fields.h3aInit ) {
        CAMHAL_LOGE("Couldn't find H3AConfig constructor");
    }

    fields.aewbMode = env->GetFieldID(metaDataClazz, "aewbMode", "I");
    if ( NULL == fields.aewbMode ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.aewbThresholdPixelValue = env->GetFieldID(metaDataClazz, "aewbThresholdPixelValue", "I");
    if ( NULL == fields.aewbThresholdPixelValue ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.aewbAccumulationShift = env->GetFieldID(metaDataClazz, "aewbAccumulationShift", "I");
    if ( NULL == fields.aewbAccumulationShift ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.aewbPaxelWindow = env->GetFieldID(metaDataClazz,
                                             "aewbPaxelWindow",
                                             "Lcom/ti/omap/android/cpcam/CPCamMetadata$H3AConfig;");
    if ( NULL == fields.aewbPaxelWindow ) {
        CAMHAL_LOGE("Couldn't allocate Metadata field");
    }

    fields.lscTable = env->GetFieldID(metaDataClazz, "lscTable", "Ljava/nio/ByteBuffer;");
    if ( NULL == fields.lscTable ) {
        CAMHAL_LOGE("Couldn't find Metadata field");
    }

    fields.auxImage = env->GetFieldID(metaDataClazz, "auxImage", "Ljava/nio/ByteBuffer;");
    if ( NULL == fields.auxImage ) {
        CAMHAL_LOGE("Couldn't find Metadata field");
    }

    fields.bscRowSum = env->GetFieldID(metaDataClazz, "bscRowSum", "Ljava/nio/ByteBuffer;");
    if ( NULL == fields.bscRowSum ) {
        CAMHAL_LOGE("Couldn't find Metadata field");
    }

    fields.bscColSum = env->GetFieldID(metaDataClazz, "bscColSum", "Ljava/nio/ByteBuffer;");
    if ( NULL == fields.bscColSum ) {
        CAMHAL_LOGE("Couldn't find Metadata field");
    }

    fields.afPaxelStatistics = env->GetFieldID(metaDataClazz,
                                               "afPaxelStatistics",
                                               "Ljava/nio/ByteBuffer;");
    if ( NULL == fields.afPaxelStatistics ) {
        CAMHAL_LOGE("Couldn't find Metadata field");
    }

    fields.aewbPaxelStatistics = env->GetFieldID(metaDataClazz,
                                               "aewbPaxelStatistics",
                                               "Ljava/nio/ByteBuffer;");
    if ( NULL == fields.aewbPaxelStatistics ) {
        CAMHAL_LOGE("Couldn't find Metadata field");
    }

}

static jobject Metadata_retrieveMetadata(JNIEnv* env, jclass clazz, jobject st, jint slot)
{

    jclass stClazz = env->FindClass(kMetadataAccessClassPathName);
    if (stClazz == NULL) {
        return NULL;
    }

    jfieldID context = env->GetFieldID(stClazz, kMetadataAccessJNIID, "I");
    if ( context == NULL ) {
        return NULL;
    }

    sp<METADATA_ACCESS_TYPE> access = NULL;
    if ( st != NULL ) {
        access = reinterpret_cast<METADATA_ACCESS_TYPE *>(env->GetIntField(st, context));
    }

#ifdef ANDROID_API_JB_OR_LATER
    BufferQueue::BufferItem item;
    access->getBuffer(slot, &item);
    sp<IMemory> data = item.mMetadata;
#else
    sp<IMemory> data = access->getMetadata();
#endif

    ssize_t offset;
    size_t size;

    if ( NULL == data.get() ) {
        return NULL;
    }

    sp<IMemoryHeap> heap = data->getMemory(&offset, &size);
    camera_metadata_t * meta = static_cast<camera_metadata_t *> (heap->base());

    jclass h3aConfigClazz = env->FindClass("com/ti/omap/android/cpcam/CPCamMetadata$H3AConfig");
    if ( NULL == h3aConfigClazz ) {
        CAMHAL_LOGE("Couldn't find H3AConfig class");
        return NULL;
    }

    jclass metaDataClazz = env->FindClass(kMetadataClassPathName);
    if ( NULL == metaDataClazz ) {
        CAMHAL_LOGE("Couldn't find Metadata class");
        return NULL;
    }

    jobject objMeta = (jobject) env->NewObject(metaDataClazz,
                                               fields.metadataInit);
    if ( NULL == objMeta ) {
        CAMHAL_LOGE("Couldn't allocate Metadata object");
        return NULL;
    }

    if ( 0 < meta->lsc_table_size ) {
        jobject nioLSCTable = env->NewDirectByteBuffer((uint8_t *)meta + meta->lsc_table_offset,
                                                  meta->lsc_table_size);
        if ( NULL == nioLSCTable ) {
            CAMHAL_LOGE("Couldn't allocate NIO LSC table");
            return NULL;
        }
        env->SetObjectField(objMeta, fields.lscTable, nioLSCTable);
        env->DeleteLocalRef(nioLSCTable);
        env->SetIntField(objMeta, fields.lscTableApplied, meta->lsc_table_applied);
    }

    jobject nioFaces = env->NewDirectByteBuffer((uint8_t *)meta + meta->faces_offset,
                                              meta->number_of_faces * sizeof(camera_metadata_face_t));
    if ( NULL == nioFaces ) {
        CAMHAL_LOGE("Couldn't allocate NIO Face array");
        return NULL;
    }
    env->SetObjectField(objMeta, fields.faces, nioFaces);
    env->DeleteLocalRef(nioFaces);
    env->SetIntField(objMeta, fields.numberOfFaces, meta->number_of_faces);

    env->SetIntField(objMeta, fields.frameNumber, meta->frame_number);
    env->SetIntField(objMeta, fields.shotNumber, meta->shot_number);
    env->SetIntField(objMeta, fields.analogGain, meta->analog_gain);
    env->SetIntField(objMeta, fields.analogGainReq, meta->analog_gain_req);
    env->SetIntField(objMeta, fields.analogGainMin, meta->analog_gain_min);
    env->SetIntField(objMeta, fields.analogGainMax, meta->analog_gain_max);
    env->SetIntField(objMeta, fields.analogGainDev, meta->analog_gain_dev);
    env->SetIntField(objMeta, fields.analogGainError, meta->analog_gain_error);
    env->SetIntField(objMeta, fields.exposureTime, meta->exposure_time);
    env->SetIntField(objMeta, fields.exposureTimeReq, meta->exposure_time_req);
    env->SetIntField(objMeta, fields.exposureTimeMin, meta->exposure_time_min);
    env->SetIntField(objMeta, fields.exposureTimeMax, meta->exposure_time_max);
    env->SetIntField(objMeta, fields.exposureTimeDev, meta->exposure_time_dev);
    env->SetIntField(objMeta, fields.exposureTimeError, meta->exposure_time_error);
    env->SetIntField(objMeta, fields.exposureCompensationReq, meta->exposure_compensation_req);
    env->SetIntField(objMeta, fields.exposureDev, meta->exposure_dev);
    env->SetLongField(objMeta, fields.timestamp, meta->timestamp);
    env->SetIntField(objMeta, fields.awbTemp, meta->awb_temp);
    env->SetIntField(objMeta, fields.gainR, meta->gain_r);
    env->SetIntField(objMeta, fields.gainGR, meta->gain_gr);
    env->SetIntField(objMeta, fields.gainGB, meta->gain_gb);
    env->SetIntField(objMeta, fields.gainB, meta->gain_b);
    env->SetIntField(objMeta, fields.offsetR, meta->offset_r);
    env->SetIntField(objMeta, fields.offsetGR, meta->offset_gr);
    env->SetIntField(objMeta, fields.offsetGB, meta->offset_gb);
    env->SetIntField(objMeta, fields.offsetB, meta->offset_b);

    return objMeta;
}

static JNINativeMethod gMetadataMethods[] = {
    {"nativeClassInit",          "()V", (void*)Metadata_Init },
    {"nativeRetrieveMetadata",   "(Lcom/ti/omap/android/cpcam/CPCamBufferQueue;I)Lcom/ti/omap/android/cpcam/CPCamMetadata;", (void*)Metadata_retrieveMetadata },
};

int register_com_ti_omap_android_cpcam_CPCamMetadata(JNIEnv* env)
{
    int err = 0;
    err = AndroidRuntime::registerNativeMethods(env,
                                                kMetadataClassPathName,
                                                gMetadataMethods,
                                                NELEM(gMetadataMethods));
    return err;
}
