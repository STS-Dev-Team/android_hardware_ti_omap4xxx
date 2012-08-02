/*
 * Copyright (C) 2010 The Android Open Source Project
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

#define LOG_TAG "BufferQueue"

#include <stdio.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gui/BufferQueue.h>
#include <gui/SurfaceTextureClient.h>

#include <android_runtime/AndroidRuntime.h>

#include <utils/Log.h>
#include <utils/misc.h>

#include "jni.h"
#include "JNIHelp.h"

#define CAMHAL_LOGV ALOGV
#define CAMHAL_LOGE ALOGE

// ----------------------------------------------------------------------------
using namespace android;

static const char* const OutOfResourcesException =
    "com/ti/omap/android/cpcam/CPCamBufferQueue$OutOfResourcesException";
static const char* const IllegalStateException = "java/lang/IllegalStateException";
const char* const kCPCamBufferQueueClassPathName = "com/ti/omap/android/cpcam/CPCamBufferQueue";

struct fields_t {
    jfieldID  surfaceTexture;
    jmethodID postEvent;
    jfieldID  rect_left;
    jfieldID  rect_top;
    jfieldID  rect_right;
    jfieldID  rect_bottom;
    jfieldID  bufferQueue;
    jfieldID  context;
};
static fields_t fields;

// ----------------------------------------------------------------------------

static void CPCamBufferQueue_setCPCamBufferQueue(JNIEnv* env, jobject thiz,
        const sp<BufferQueue>& bufferQueue)
{
    BufferQueue* const p =
        (BufferQueue*)env->GetIntField(thiz, fields.bufferQueue);
    if (bufferQueue.get()) {
        bufferQueue->incStrong(thiz);
    }
    if (p) {
        p->decStrong(thiz);
    }
    env->SetIntField(thiz, fields.bufferQueue, (int)bufferQueue.get());
}

sp<BufferQueue> CPCamBufferQueue_getCPCamBufferQueue(JNIEnv* env, jobject thiz)
{
    sp<BufferQueue> bufferQueue(
        (BufferQueue*)env->GetIntField(thiz, fields.bufferQueue));
    return bufferQueue;
}

sp<ANativeWindow> android_CPCamBufferQueue_getNativeWindow(
        JNIEnv* env, jobject thiz)
{
    sp<ISurfaceTexture> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));
    sp<SurfaceTextureClient> surfaceTextureClient(bufferQueue != NULL ?
            new SurfaceTextureClient(bufferQueue) : NULL);
    return surfaceTextureClient;
}

bool android_CPCamBufferQueue_isInstanceOf(JNIEnv* env, jobject thiz)
{
    jclass bufferQueueClass = env->FindClass(kCPCamBufferQueueClassPathName);
    return env->IsInstanceOf(thiz, bufferQueueClass);
}

// ----------------------------------------------------------------------------

class JNICPCamBufferQueueContext : public BufferQueue::ProxyConsumerListener
{
public:
    JNICPCamBufferQueueContext(JNIEnv* env, jobject weakThiz, jclass clazz);
    virtual ~JNICPCamBufferQueueContext();
    virtual void onFrameAvailable();
    virtual void onBuffersReleased();
    void saveBuffer(unsigned int slot, sp<GraphicBuffer> gbuf);
    sp<GraphicBuffer> retrieveBuffer(unsigned int slot);

private:
    static JNIEnv* getJNIEnv(bool* needsDetach);
    static void detachJNI();

    jobject mWeakThiz;
    jclass mClazz;

    BufferQueue::BufferItem mBufferSlots[BufferQueue::NUM_BUFFER_SLOTS];
};

JNICPCamBufferQueueContext::JNICPCamBufferQueueContext(JNIEnv* env,
        jobject weakThiz, jclass clazz) :
    BufferQueue::ProxyConsumerListener(NULL),
    mWeakThiz(env->NewGlobalRef(weakThiz)),
    mClazz((jclass)env->NewGlobalRef(clazz))
{}

JNIEnv* JNICPCamBufferQueueContext::getJNIEnv(bool* needsDetach) {
    *needsDetach = false;
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (env == NULL) {
        JavaVMAttachArgs args = {JNI_VERSION_1_4, NULL, NULL};
        JavaVM* vm = AndroidRuntime::getJavaVM();
        int result = vm->AttachCurrentThread(&env, (void*) &args);
        if (result != JNI_OK) {
            ALOGE("thread attach failed: %#x", result);
            return NULL;
        }
        *needsDetach = true;
    }
    return env;
}

void JNICPCamBufferQueueContext::detachJNI() {
    JavaVM* vm = AndroidRuntime::getJavaVM();
    int result = vm->DetachCurrentThread();
    if (result != JNI_OK) {
        ALOGE("thread detach failed: %#x", result);
    }
}

JNICPCamBufferQueueContext::~JNICPCamBufferQueueContext()
{
    bool needsDetach = false;
    JNIEnv* env = getJNIEnv(&needsDetach);
    if (env != NULL) {
        env->DeleteGlobalRef(mWeakThiz);
        env->DeleteGlobalRef(mClazz);
    } else {
        ALOGW("leaking JNI object references");
    }
    if (needsDetach) {
        detachJNI();
    }
}

void JNICPCamBufferQueueContext::onFrameAvailable()
{
    bool needsDetach = false;
    JNIEnv* env = getJNIEnv(&needsDetach);
    if (env != NULL) {
        env->CallStaticVoidMethod(mClazz, fields.postEvent, mWeakThiz);
    } else {
        ALOGW("onFrameAvailable event will not posted");
    }
    if (needsDetach) {
        detachJNI();
    }
}

void JNICPCamBufferQueueContext::onBuffersReleased()
{
    for (int i = 0; i < BufferQueue::NUM_BUFFER_SLOTS; i++) {
        mBufferSlots[i].mGraphicBuffer = 0;
    }
}

void JNICPCamBufferQueueContext::saveBuffer(unsigned int slot, sp<GraphicBuffer> gbuf)
{
    if (slot < BufferQueue::NUM_BUFFER_SLOTS) {
        mBufferSlots[slot].mGraphicBuffer = gbuf;
    }
}

sp<GraphicBuffer> JNICPCamBufferQueueContext::retrieveBuffer(unsigned int slot)
{
    sp<GraphicBuffer> gbuf = 0;

    if (slot < BufferQueue::NUM_BUFFER_SLOTS) {
        gbuf = mBufferSlots[slot].mGraphicBuffer;
    }

    return gbuf;
}
// ----------------------------------------------------------------------------

static void CPCamBufferQueue_classInit(JNIEnv* env, jclass clazz)
{
    fields.bufferQueue = env->GetFieldID(clazz, "mBufferQueue", "I");
    if (fields.bufferQueue == NULL) {
        ALOGE("can't find com/ti/omap/android/cpcam/CPCamBufferQueue.%s",
                "mBufferQueue");
    }

    fields.postEvent = env->GetStaticMethodID(clazz, "postEventFromNative",
            "(Ljava/lang/Object;)V");
    if (fields.postEvent == NULL) {
        ALOGE("can't find com/ti/omap/android/cpcam/CPCamBufferQueue.postEventFromNative");
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    if (fields.context == NULL) {
        ALOGE("can't find com/ti/omap/android/cpcam/CPCamBufferQueue.%s",
                "mNativeContext");
    }
    ALOGE("CPCamBufferQueue_classInit");
}

static void CPCamBufferQueue_init(JNIEnv* env, jobject thiz,
        jobject weakThiz, jboolean allowSynchronous)
{
    sp<BufferQueue> bufferQueue(new BufferQueue(allowSynchronous, 1));
    if (bufferQueue == 0) {
        jniThrowException(env, OutOfResourcesException,
                "Unable to create native SurfaceTexture");
        return;
    }
    CPCamBufferQueue_setCPCamBufferQueue(env, thiz, bufferQueue);

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        jniThrowRuntimeException(env,
                "Can't find com/ti/omap/android/cpcam/SurfaceTexture");
        return;
    }

    sp<JNICPCamBufferQueueContext> ctx(new JNICPCamBufferQueueContext(env, weakThiz,
            clazz));

    status_t err = bufferQueue->consumerConnect(ctx);
    if (err != NO_ERROR) {
        jniThrowRuntimeException(env,
                "error connecting to BufferQueue");
        return;
    }

    // save context in opaque field
    env->SetIntField(thiz, fields.context, (int)ctx.get());

    // TODO(XXX): Need to figure out if we need to set these
    // mBufferQueue->setConsumerName(mName);
    // mBufferQueue->setConsumerUsageBits(DEFAULT_USAGE_FLAGS);
}

static void CPCamBufferQueue_finalize(JNIEnv* env, jobject thiz)
{
    sp<BufferQueue> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));
    bufferQueue->consumerDisconnect();
    CPCamBufferQueue_setCPCamBufferQueue(env, thiz, 0);

    // Delete reference to context
    env->SetIntField(thiz, fields.context, 0);
}

static void CPCamBufferQueue_setDefaultBufferSize(
        JNIEnv* env, jobject thiz, jint width, jint height)
{
    sp<BufferQueue> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));
    bufferQueue->setDefaultBufferSize(width, height);
}

static jint CPCamBufferQueue_acquireBuffer(JNIEnv* env, jobject thiz)
{
    sp<BufferQueue> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));
    JNICPCamBufferQueueContext *ctx = NULL;

    BufferQueue::BufferItem item;

    status_t err = bufferQueue->acquireBuffer(&item);
    if (err == INVALID_OPERATION) {
        jniThrowException(env, IllegalStateException, "Unable to take reference to buffer (see "
                "logcat for details)");
    } else if (err != NO_ERROR) {
        jniThrowRuntimeException(env, "Error during takeCurrentBuffer (see logcat for details)");
    }

    ctx = reinterpret_cast<JNICPCamBufferQueueContext*>(env->GetIntField(thiz, fields.context));

    // Need to hold a reference to newly allocated buffers
    // mGraphicBuffer field is only filled the first time
    // we acquire the buffer
    if (ctx != NULL && item.mGraphicBuffer != NULL) {
        ctx->saveBuffer(item.mBuf, item.mGraphicBuffer);
    }

    return item.mBuf;
}

static void CPCamBufferQueue_releaseBuffer(JNIEnv* env, jobject thiz, jint slot)
{
    sp<BufferQueue> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));

    bufferQueue->releaseBuffer(slot, EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
}

static void CPCamBufferQueue_getCropRect(JNIEnv* env, jobject thiz,
        jint slot, jobject rect)
{
    sp<BufferQueue> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));
    BufferQueue::BufferItem item;

    status_t err = bufferQueue->getBuffer(slot, &item);
    if (err == INVALID_OPERATION) {
        jniThrowException(env, IllegalStateException, "Unable to take reference to buffer (see "
                "logcat for details)");
    } else if (err != NO_ERROR) {
        jniThrowRuntimeException(env, "Error during takeCurrentBuffer (see logcat for details)");
    }

    jclass clazz = env->GetObjectClass(rect);
    if (clazz != 0) {
        env->SetIntField(rect, fields.rect_left, item.mCrop.left);
        env->SetIntField(rect, fields.rect_top, item.mCrop.top);
        env->SetIntField(rect, fields.rect_right, item.mCrop.right);
        env->SetIntField(rect, fields.rect_bottom, item.mCrop.bottom);
    }
}

static jlong CPCamBufferQueue_getTimestamp(JNIEnv* env, jobject thiz, jint slot)
{
    sp<BufferQueue> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));
    BufferQueue::BufferItem item;

    status_t err = bufferQueue->getBuffer(slot, &item);
    if (err == INVALID_OPERATION) {
        jniThrowException(env, IllegalStateException, "Unable to take reference to buffer (see "
                "logcat for details)");
    } else if (err != NO_ERROR) {
        jniThrowRuntimeException(env, "Error during takeCurrentBuffer (see logcat for details)");
    }

    return item.mTimestamp;
}

static void CPCamBufferQueue_release(JNIEnv* env, jobject thiz)
{
    sp<BufferQueue> bufferQueue(CPCamBufferQueue_getCPCamBufferQueue(env, thiz));
    bufferQueue->consumerDisconnect();

    // Delete reference to context
    env->SetIntField(thiz, fields.context, 0);
}

// ----------------------------------------------------------------------------

static JNINativeMethod gCPCamBufferQueueMethods[] = {
    {"nativeClassInit",            "()V",   (void*)CPCamBufferQueue_classInit },
    {"nativeInit",                 "(Ljava/lang/Object;Z)V", (void*)CPCamBufferQueue_init },
    {"nativeFinalize",             "()V",   (void*)CPCamBufferQueue_finalize },
    {"nativeSetDefaultBufferSize", "(II)V", (void*)CPCamBufferQueue_setDefaultBufferSize },
    {"nativeAcquireBuffer",        "()I",   (void*)CPCamBufferQueue_acquireBuffer },
    {"nativeReleaseBuffer",        "(I)V",  (void*)CPCamBufferQueue_releaseBuffer },
    {"nativeGetTimestamp",         "(I)J",  (void*)CPCamBufferQueue_getTimestamp },
    {"nativeRelease",              "()V",   (void*)CPCamBufferQueue_release },
};

struct field {
    const char *class_name;
    const char *field_name;
    const char *field_type;
    jfieldID   *jfield;
};

static int find_fields(JNIEnv *env, field *fields, int count)
{
    for (int i = 0; i < count; i++) {
        field *f = &fields[i];
        jclass clazz = env->FindClass(f->class_name);
        if (clazz == NULL) {
            CAMHAL_LOGE("Can't find %s", f->class_name);
            return -1;
        }

        jfieldID field = env->GetFieldID(clazz, f->field_name, f->field_type);
        if (field == NULL) {
            CAMHAL_LOGE("Can't find %s.%s", f->class_name, f->field_name);
            return -1;
        }

        *(f->jfield) = field;
    }

    return 0;
}

int register_android_graphics_CPCamBufferQueue(JNIEnv* env)
{
    int err = 0;
    jclass clazz;

    field fields_to_find[] = {
        { "android/graphics/Rect", "left", "I", &fields.rect_left },
        { "android/graphics/Rect", "top", "I", &fields.rect_top },
        { "android/graphics/Rect", "right", "I", &fields.rect_right },
        { "android/graphics/Rect", "bottom", "I", &fields.rect_bottom },
    };

    if (find_fields(env, fields_to_find, NELEM(fields_to_find)) < 0)
        return -1;

    clazz = env->FindClass(kCPCamBufferQueueClassPathName);
    if (env->RegisterNatives(clazz, gCPCamBufferQueueMethods,
            NELEM(gCPCamBufferQueueMethods)) != JNI_OK)
    {
        ALOGE("Failed registering methods for %s\n", kCPCamBufferQueueClassPathName);
        return -1;
    }

    return err;
}
