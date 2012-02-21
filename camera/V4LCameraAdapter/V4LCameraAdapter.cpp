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
* @file V4LCameraAdapter.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/


#include "V4LCameraAdapter.h"
#include "CameraHal.h"
#include "TICameraParameters.h"
#include "DebugUtils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev.h>

#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include <cutils/properties.h>
#define UNLIKELY( exp ) (__builtin_expect( (exp) != 0, false ))
static int mDebugFps = 0;

#define Q16_OFFSET 16

#define HERE(Msg) {CAMHAL_LOGEB("--===line %d, %s===--\n", __LINE__, Msg);}

namespace android {

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

//define this macro to save first few raw frames when starting the preview.
//#define SAVE_RAW_FRAMES 1

Mutex gV4LAdapterLock;
const char *device = DEVICE;


/*--------------------Camera Adapter Class STARTS here-----------------------------*/

status_t V4LCameraAdapter::initialize(CameraProperties::Properties* caps)
{
    LOG_FUNCTION_NAME;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);

    int ret = NO_ERROR;

    // Allocate memory for video info structure
    mVideoInfo = (struct VideoInfo *) calloc (1, sizeof (struct VideoInfo));
    if(!mVideoInfo)
        {
        return NO_MEMORY;
        }

    if ((mCameraHandle = open(device, O_RDWR)) == -1)
        {
        CAMHAL_LOGEB("Error while opening handle to V4L2 Camera: %s", strerror(errno));
        return -EINVAL;
        }

    ret = ioctl (mCameraHandle, VIDIOC_QUERYCAP, &mVideoInfo->cap);
    if (ret < 0)
        {
        CAMHAL_LOGEA("Error when querying the capabilities of the V4L Camera");
        return -EINVAL;
        }

    if ((mVideoInfo->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
        {
        CAMHAL_LOGEA("Error while adapter initialization: video capture not supported.");
        return -EINVAL;
        }

    if (!(mVideoInfo->cap.capabilities & V4L2_CAP_STREAMING))
        {
        CAMHAL_LOGEA("Error while adapter initialization: Capture device does not support streaming i/o");
        return -EINVAL;
        }

    // Initialize flags
    mPreviewing = false;
    mVideoInfo->isStreaming = false;
    mRecording = false;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType)
{

    status_t ret = NO_ERROR;

    if ( !mVideoInfo->isStreaming )
        {
        return NO_ERROR;
        }

    int i = mPreviewBufs.valueFor(( unsigned int )frameBuf);
    if(i<0)
        {
        return BAD_VALUE;
        }

    mVideoInfo->buf.index = i;
    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
    if (ret < 0) {
       CAMHAL_LOGEA("Init: VIDIOC_QBUF Failed");
       return -1;
    }

     nQueued++;

    return ret;

}

status_t V4LCameraAdapter::setParameters(const CameraParameters &params)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;

    int width, height;

    params.getPreviewSize(&width, &height);

    CAMHAL_LOGDB("Width * Height %d x %d format 0x%x", width, height, DEFAULT_PIXEL_FORMAT);

    mVideoInfo->width = width;
    mVideoInfo->height = height;
    mVideoInfo->framesizeIn = (width * height << 1);
    mVideoInfo->formatIn = DEFAULT_PIXEL_FORMAT;

    mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->format.fmt.pix.width = width;
    mVideoInfo->format.fmt.pix.height = height;
    mVideoInfo->format.fmt.pix.pixelformat = DEFAULT_PIXEL_FORMAT;

    ret = ioctl(mCameraHandle, VIDIOC_S_FMT, &mVideoInfo->format);
    if (ret < 0) {
        CAMHAL_LOGEB("Open: VIDIOC_S_FMT Failed: %s", strerror(errno));
        return ret;
    }

    // Udpate the current parameter set
    mParams = params;

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}


void V4LCameraAdapter::getParameters(CameraParameters& params)
{
    LOG_FUNCTION_NAME;

    // Return the current parameter set
    params = mParams;

    LOG_FUNCTION_NAME_EXIT;
}


///API to give the buffers to Adapter
status_t V4LCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    switch(mode)
        {
        case CAMERA_PREVIEW:
            ret = UseBuffersPreview(bufArr, num);
            break;

        //@todo Insert Image capture case here

        case CAMERA_VIDEO:
            //@warn Video capture is not fully supported yet
            ret = UseBuffersPreview(bufArr, num);
            break;

        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::UseBuffersPreview(void* bufArr, int num)
{
    int ret = NO_ERROR;
    uint32_t *ptr = (uint32_t*) bufArr;

    if(NULL == bufArr) {
        return BAD_VALUE;
    }

    //First allocate adapter internal buffers at V4L level for USB Cam
    //These are the buffers from which we will copy the data into overlay buffers
    /* Check if camera can handle NB_BUFFER buffers */
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
    mVideoInfo->rb.count = num;

    ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }

    for (int i = 0; i < num; i++) {

        memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));

        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        mVideoInfo->mem[i] = mmap (0,
               mVideoInfo->buf.length,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               mCameraHandle,
               mVideoInfo->buf.m.offset);

        if (mVideoInfo->mem[i] == MAP_FAILED) {
            CAMHAL_LOGEB("Unable to map buffer (%s)", strerror(errno));
            return -1;
        }

        //Associate each Camera internal buffer with the one from Overlay
        mPreviewBufs.add((int)ptr[i], i);
    }

    // Update the preview buffer count
    mPreviewBufferCount = num;

    return ret;
}

status_t V4LCameraAdapter::startPreview()
{
  status_t ret = NO_ERROR;

  Mutex::Autolock lock(mPreviewBufsLock);

  if(mPreviewing) {
    return BAD_VALUE;
  }

   for (int i = 0; i < mPreviewBufferCount; i++) {

       mVideoInfo->buf.index = i;
       mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
       mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

       ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
       if (ret < 0) {
           CAMHAL_LOGEA("VIDIOC_QBUF Failed");
           return -EINVAL;
       }

       nQueued++;
   }

   enum v4l2_buf_type bufType;
   if (!mVideoInfo->isStreaming) {
       bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

       ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);
       if (ret < 0) {
           CAMHAL_LOGEB("StartStreaming: Unable to start capture: %s", strerror(errno));
           return ret;
       }

       mVideoInfo->isStreaming = true;
   }

   // Create and start preview thread for receiving buffers from V4L Camera
   mPreviewThread = new PreviewThread(this);

   CAMHAL_LOGDA("Created preview thread");

   //Update the flag to indicate we are previewing
   mPreviewing = true;

   return ret;
}

status_t V4LCameraAdapter::stopPreview()
{
    enum v4l2_buf_type bufType;
    int ret = NO_ERROR;

    Mutex::Autolock lock(mPreviewBufsLock);

    if(!mPreviewing)
        {
        return NO_INIT;
        }

    if (mVideoInfo->isStreaming) {
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
        if (ret < 0) {
            CAMHAL_LOGEB("StopStreaming: Unable to stop capture: %s", strerror(errno));
            return ret;
        }

        mVideoInfo->isStreaming = false;
    }

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    nQueued = 0;
    nDequeued = 0;

    /* Unmap buffers */
    for (int i = 0; i < mPreviewBufferCount; i++)
        if (munmap(mVideoInfo->mem[i], mVideoInfo->buf.length) < 0)
            CAMHAL_LOGEA("Unmap failed");

    mPreviewBufs.clear();

    mPreviewThread->requestExitAndWait();
    mPreviewThread.clear();

    return ret;
}

char * V4LCameraAdapter::GetFrame(int &index)
{
    int ret;

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    /* DQ */
    ret = ioctl(mCameraHandle, VIDIOC_DQBUF, &mVideoInfo->buf);
    if (ret < 0) {
        CAMHAL_LOGEA("GetFrame: VIDIOC_DQBUF Failed");
        return NULL;
    }
    nDequeued++;

    index = mVideoInfo->buf.index;

    return (char *)mVideoInfo->mem[mVideoInfo->buf.index];
}

//API to get the frame size required to be allocated. This size is used to override the size passed
//by camera service when VSTAB/VNF is turned ON for example
status_t V4LCameraAdapter::getFrameSize(size_t &width, size_t &height)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    // Just return the current preview size, nothing more to do here.
    mParams.getPreviewSize(( int * ) &width,
                           ( int * ) &height);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    // We don't support meta data, so simply return
    return NO_ERROR;
}

status_t V4LCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    // We don't support image capture yet, safely return from here without messing up
    return NO_ERROR;
}

static void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if (!(mFrameCount & 0x1F)) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        LOGD("Camera %d Frames, %f FPS", mFrameCount, mFps);
    }
    // XXX: mFPS has the value we want
}

status_t V4LCameraAdapter::recalculateFPS()
{
    float currentFPS;

    mFrameCount++;

    if ( ( mFrameCount % FPS_PERIOD ) == 0 )
        {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter )
            {
            mFPS = currentFPS;
            }
        else
            {
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
            }

        mLastFPS = mFPS;
        mIter++;
        }

    return NO_ERROR;
}

void V4LCameraAdapter::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;
}


V4LCameraAdapter::V4LCameraAdapter(size_t sensor_index)
{
    LOG_FUNCTION_NAME;

    // Nothing useful to do in the constructor

    LOG_FUNCTION_NAME_EXIT;
}

V4LCameraAdapter::~V4LCameraAdapter()
{
    LOG_FUNCTION_NAME;

    // Close the camera handle and free the video info structure
    close(mCameraHandle);

    if (mVideoInfo)
      {
        free(mVideoInfo);
        mVideoInfo = NULL;
      }

    LOG_FUNCTION_NAME_EXIT;
}

void convertYUV422ToNV12(unsigned char *src, unsigned char *dest, int width, int height ) {
    //convert YUV422I to YUV420 NV12 format.
    size_t        nv12_buf_size = (width * height)*3/2;
    unsigned char *bf = src;
    unsigned char *dst = dest;

    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            *dst = *bf;
             dst++;
             bf = bf + 2;
        }
    }

    bf = src;
    bf++;  //U sample
    for(int i = 0; i < height/2; i++)
    {
        for(int j=0; j<width; j++)
        {
            *dst = *bf;
             dst++;
             bf = bf + 2;
        }
        bf = bf + width*2;
    }
}

#ifdef SAVE_RAW_FRAMES
void saveFile(unsigned char* buff, int buff_size) {
    static int      counter = 1;
    int             fd = -1;
    char            fn[256];

    LOG_FUNCTION_NAME;
    if (counter > 3) {
        return;
    }
    //dump nv12 buffer
    counter++;
    sprintf(fn, "/data/misc/camera/raw/nv12_dump_%03d.yuv", counter);
    CAMHAL_LOGEB("Dumping nv12 frame to a file : %s.", fn);

    fd = open(fn, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
    if(fd < 0) {
        LOGE("Unable to open file %s: %s", fn, strerror(fd));
        return;
    }

    write(fd, buff, buff_size );
    close(fd);

    LOG_FUNCTION_NAME_EXIT;
}
#endif

/* Preview Thread */
// ---------------------------------------------------------------------------

int V4LCameraAdapter::previewThread()
{
    status_t ret = NO_ERROR;
    int width, height;
    CameraFrame frame;
    unsigned char *nv12_buff = NULL;
    void *y_uv[2];

    mParams.getPreviewSize(&width, &height);
    nv12_buff = (unsigned char*) malloc(width*height*3/2);

    if (mPreviewing) {
        int index = 0;
        char *fp = this->GetFrame(index);
        if(!fp) {
            return BAD_VALUE;
        }

        uint8_t* ptr = (uint8_t*) mPreviewBufs.keyAt(index);
        int stride = 4096;

        //Convert yuv422i ti yuv420sp(NV12) & dump the frame to a file
        convertYUV422ToNV12 ( (unsigned char*)fp, nv12_buff, width, height);
#ifdef SAVE_RAW_FRAMES
        saveFile( nv12_buff, ((width*height)*3/2) );
#endif

        CameraFrame *lframe = (CameraFrame *)mFrameQueue.valueFor(ptr);
        y_uv[0] = (void*) lframe->mYuv[0];
        //y_uv[1] = (void*) lframe->mYuv[1];
        y_uv[1] = (void*) lframe->mYuv[0] + height*stride;

        CAMHAL_LOGVB("##...index= %d.;ptr= 0x%x; y= 0x%x; UV= 0x%x.",index, ptr, y_uv[0], y_uv[1] );

        unsigned char *bufferDst = ( unsigned char * ) y_uv[0];
        unsigned char *bufferSrc = nv12_buff;
        int rowBytes = width;

        //Copy the Y plane to Gralloc buffer
        for(int i = 0; i < height; i++) {
            memcpy(bufferDst, bufferSrc, rowBytes);
            bufferDst += stride;
            bufferSrc += rowBytes;
        }
        //Copy UV plane, now Y & UV are contiguous.
        //bufferDst = ( unsigned char * ) y_uv[1];
        for(int j = 0; j < height/2; j++) {
            memcpy(bufferDst, bufferSrc, rowBytes);
            bufferDst += stride;
            bufferSrc += rowBytes;
        }

        frame.mFrameType = CameraFrame::PREVIEW_FRAME_SYNC;
        frame.mBuffer = ptr;
        frame.mLength = width*height*3/2;
        frame.mAlignment = stride;
        frame.mOffset = 0;
        frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);;
        frame.mFrameMask = (unsigned int)CameraFrame::PREVIEW_FRAME_SYNC;

        ret = setInitFrameRefCount(frame.mBuffer, frame.mFrameMask);
        if (ret != NO_ERROR) {
            CAMHAL_LOGDB("Error in setInitFrameRefCount %d", ret);
        } else {
            ret = sendFrameToSubscribers(&frame);
        }
    }

    free (nv12_buff);
    return ret;
}

extern "C" CameraAdapter* V4LCameraAdapter_Factory(size_t sensor_index)
{
    CameraAdapter *adapter = NULL;
    Mutex::Autolock lock(gV4LAdapterLock);

    LOG_FUNCTION_NAME;

    adapter = new V4LCameraAdapter(sensor_index);
    if ( adapter ) {
        CAMHAL_LOGDB("New V4L Camera adapter instance created for sensor %d",sensor_index);
    } else {
        CAMHAL_LOGEA("V4L Camera adapter create failed for sensor index = %d!",sensor_index);
    }

    LOG_FUNCTION_NAME_EXIT;

    return adapter;
}

extern "C" status_t V4LCameraAdapter_Capabilities(
        CameraProperties::Properties * const properties_array,
        const int starting_camera, const int max_camera, int & supportedCameras)
{
    status_t ret = NO_ERROR;
    struct v4l2_capability cap;
    int tempHandle = NULL;
    int num_cameras_supported = 0;
    CameraProperties::Properties* properties = NULL;

    LOG_FUNCTION_NAME;

    supportedCameras = 0;
    memset((void*)&cap, 0, sizeof(v4l2_capability));

    if (!properties_array) {
        CAMHAL_LOGEB("invalid param: properties = 0x%p", properties_array);
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    //look for the connected video devices
    if (ret == NO_ERROR && (starting_camera + num_cameras_supported) < max_camera) {

        if ((tempHandle = open(device, O_RDWR)) == -1) {
            CAMHAL_LOGEB("Error while opening handle to V4L2 Camera: %s", strerror(errno));
            return NO_ERROR;
        }

        ret = ioctl (tempHandle, VIDIOC_QUERYCAP, &cap);
        if (ret < 0)
        {
            CAMHAL_LOGEA("Error when querying the capabilities of the V4L Camera");
            goto EXIT;
        }

        //check for video capture devices
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
        {
            CAMHAL_LOGEA("Error while adapter initialization: video capture not supported.");
            goto EXIT;
        }

        num_cameras_supported++;
        properties = properties_array + starting_camera;

        // TODO: Need to tell camera properties what other cameras we can support
        properties->set(CameraProperties::CAMERA_NAME, "USBCAMERA");
        properties->set(CameraProperties::PREVIEW_SIZE, "640x480");
        properties->set(CameraProperties::PREVIEW_FORMAT, "yuv420sp");
        properties->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS, "yuv420sp");
        properties->set(CameraProperties::PICTURE_SIZE, "640x480");
        properties->set(CameraProperties::JPEG_THUMBNAIL_SIZE, "320x240");
        properties->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, "640x480");
        properties->set(CameraProperties::SUPPORTED_PICTURE_SIZES, "640x480");
        properties->set(CameraProperties::REQUIRED_PREVIEW_BUFS, "6");
        properties->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, "30000,30000");
        properties->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, "30000,30000");
        properties->set(CameraProperties::FRAMERATE_RANGE, "30000,30000");
        properties->set(CameraProperties::PREVIEW_FRAME_RATE, "30000");

    }

    supportedCameras = num_cameras_supported;
    CAMHAL_LOGDB("V4L cameras detected =%d", num_cameras_supported);

EXIT:
    LOG_FUNCTION_NAME_EXIT;
    close(tempHandle);
    return NO_ERROR;
}

};


/*--------------------Camera Adapter Class ENDS here-----------------------------*/

