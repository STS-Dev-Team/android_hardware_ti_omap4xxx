/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <climits>

#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/ISurfaceComposerClient.h>
#include <surfaceflinger/SurfaceComposerClient.h>

#include <gui/SurfaceTexture.h>
#include <gui/SurfaceTextureClient.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <camera/ShotParameters.h>
#include <camera/CameraMetadata.h>
#include <system/audio.h>
#include <system/camera.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include "camera_test.h"

#define ASSERT(X) \
    do { \
       if(!(X)) { \
           printf("error: %s():%d", __FUNCTION__, __LINE__); \
           return; \
       } \
    } while(0);

#define ALIGN_DOWN(x, n) ((x) & (~((n) - 1)))
#define ALIGN_UP(x, n) ((((x) + (n) - 1)) & (~((n) - 1)))
#define ALIGN_WIDTH 32 // Should be 32...but the calculated dimension causes an ion crash
#define ALIGN_HEIGHT 2 // Should be 2...but the calculated dimension causes an ion crash

//temporarily define format here
#define HAL_PIXEL_FORMAT_TI_NV12 0x100
#define HAL_PIXEL_FORMAT_TI_NV12_1D 0x102
#define HAL_PIXEL_FORMAT_TI_Y8 0x103
#define HAL_PIXEL_FORMAT_TI_Y16 0x104

using namespace android;


static void
test_format (int format)
{
    SurfaceTexture *st;
    SurfaceTextureClient *stc;
    GLint tex_id = 0;
    sp<ANativeWindow> anw;
    ANativeWindowBuffer* anb[30] = { 0 };
    int i;

    printf("testing format %x\n", format);

    st = new SurfaceTexture (tex_id, true, GL_TEXTURE_EXTERNAL_OES);

    st->setDefaultBufferSize (2608, 1960);

    stc = new SurfaceTextureClient(st);
    anw = stc;
    native_window_set_usage(anw.get(),
            GRALLOC_USAGE_SW_READ_RARELY |
            GRALLOC_USAGE_SW_WRITE_NEVER);
    native_window_set_buffer_count(anw.get(), 18);
    native_window_set_buffers_geometry(anw.get(),
            2608, 1960, format);

    for(i=0;i<18;i++) {
        anb[i] = NULL;
        anw->dequeueBuffer(anw.get(), &anb[i]);
        printf("%d: %p\n", i, anb[i]);
        if (anb[i] == NULL) {
            printf ("FAILED: buffer should be non-NULL\n");
        }
    }
    for(i=0;i<18;i++) {
        if (anb[i]) {
            anw->cancelBuffer (anw.get(), anb[i]);
        }
    }

    //delete stc;
    delete st;
}


int
main (int argc, char *argv[])
{
    test_format (HAL_PIXEL_FORMAT_TI_NV12_1D);
    test_format (HAL_PIXEL_FORMAT_TI_Y8);
    test_format (HAL_PIXEL_FORMAT_TI_Y16);

    return 0;
}

