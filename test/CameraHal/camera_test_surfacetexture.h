#ifndef CAMERA_TEST_SURFACE_TEXTURE_H
#define CAMERA_TEST_SURFACE_TEXTURE_H

#include "camera_test.h"

#ifdef ANDROID_API_JB_OR_LATER
#include <gui/Surface.h>
#include <gui/SurfaceTexture.h>
#include <gui/SurfaceComposerClient.h>
#else
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/ISurfaceComposerClient.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#endif

#ifdef ANDROID_API_JB_OR_LATER
#   define CAMHAL_LOGV            ALOGV
#   define CAMHAL_LOGE            ALOGE
#   define PRINTOVER(arg...)      ALOGD(#arg)
#   define LOG_FUNCTION_NAME      ALOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#   define LOG_FUNCTION_NAME_EXIT ALOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);
#else
#   define CAMHAL_LOGV            LOGV
#   define CAMHAL_LOGE            LOGE
#   define PRINTOVER(arg...)      LOGD(#arg)
#   define LOG_FUNCTION_NAME      LOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#   define LOG_FUNCTION_NAME_EXIT LOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);
#endif

using namespace android;

class FrameWaiter : public android::SurfaceTexture::FrameAvailableListener {
public:
    FrameWaiter():
            mPendingFrames(0) {
    }

    virtual ~FrameWaiter() {
        onFrameAvailable();
    }

    void waitForFrame() {
        Mutex::Autolock lock(mMutex);
        while (mPendingFrames == 0) {
            mCondition.wait(mMutex);
        }
        mPendingFrames--;
    }

    virtual void onFrameAvailable() {
        Mutex::Autolock lock(mMutex);
        mPendingFrames++;
        mCondition.signal();
    }

    int mPendingFrames;
    Mutex mMutex;
    Condition mCondition;
};

class GLSurface {
public:

    GLSurface():
            mEglDisplay(EGL_NO_DISPLAY),
            mEglSurface(EGL_NO_SURFACE),
            mEglContext(EGL_NO_CONTEXT) {
    }

    virtual ~GLSurface() {}

    void initialize(int display);
    void deinit();
    void loadShader(GLenum shaderType, const char* pSource, GLuint* outShader);
    void createProgram(const char* pVertexSource, const char* pFragmentSource,
            GLuint* outPgm);

private:
    EGLint const* getConfigAttribs();
    EGLint const* getContextAttribs();

protected:
    sp<SurfaceComposerClient> mComposerClient;
    sp<SurfaceControl> mSurfaceControl;

    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLContext mEglContext;
    EGLConfig  mGlConfig;
};

class SurfaceTextureBase  {
public:
    virtual ~SurfaceTextureBase() {}

    void initialize(int tex_id, EGLenum tex_target = EGL_NONE);
    void deinit();

    virtual sp<SurfaceTexture> getST();

protected:
    sp<SurfaceTexture> mST;
    sp<SurfaceTextureClient> mSTC;
    sp<ANativeWindow> mANW;
    int mTexId;
};

class SurfaceTextureGL : public GLSurface, public SurfaceTextureBase {
public:
    virtual ~SurfaceTextureGL() {}

    void initialize(int display, int tex_id);
    void deinit();

    // drawTexture draws the SurfaceTexture over the entire GL viewport.
    void drawTexture();

private:
    GLuint mPgm;
    GLint mPositionHandle;
    GLint mTexSamplerHandle;
    GLint mTexMatrixHandle;
};

class ST_BufferSourceThread : public BufferSourceThread {
public:
    ST_BufferSourceThread(int tex_id, sp<Camera> camera) : BufferSourceThread(camera) {
        mSurfaceTextureBase = new SurfaceTextureBase();
        mSurfaceTextureBase->initialize(tex_id);
        mSurfaceTexture = mSurfaceTextureBase->getST();
        mSurfaceTexture->setSynchronousMode(true);
        mFW = new FrameWaiter();
        mSurfaceTexture->setFrameAvailableListener(mFW);
    }
    virtual ~ST_BufferSourceThread() {
        mSurfaceTextureBase->deinit();
        delete mSurfaceTextureBase;
    }

    virtual bool threadLoop() {
        sp<GraphicBuffer> graphic_buffer;

        mFW->waitForFrame();
        if (!mDestroying) {
            mSurfaceTexture->updateTexImage();
            printf("=== Metadata for buffer %d ===\n", mCounter);
#ifndef ANDROID_API_JB_OR_LATER
            showMetadata(mSurfaceTexture->getMetadata());
#endif
            printf("\n");
            graphic_buffer = mSurfaceTexture->getCurrentBuffer();
            mDeferThread->add(graphic_buffer, mCounter++);
            restartCapture();
            return true;
        }
        return false;
    }

    virtual void requestExit() {
        Thread::requestExit();

        mDestroying = true;
        mFW->onFrameAvailable();
    }

    virtual void setBuffer() {
#ifndef ANDROID_API_JB_OR_LATER
        mCamera->setBufferSource(NULL, mSurfaceTexture);
#endif
    }

private:
    SurfaceTextureBase *mSurfaceTextureBase;
    sp<SurfaceTexture> mSurfaceTexture;
    sp<FrameWaiter> mFW;
};

class ST_BufferSourceInput : public BufferSourceInput {
public:
    ST_BufferSourceInput(int tex_id, sp<Camera> camera) :
                 BufferSourceInput(camera), mTexId(tex_id) {
        mSurfaceTexture = new SurfaceTextureBase();
    }
    virtual ~ST_BufferSourceInput() {
        delete mSurfaceTexture;
    }

    virtual void init() {
        sp<SurfaceTexture> surface_texture;
        mSurfaceTexture->initialize(mTexId);
        surface_texture = mSurfaceTexture->getST();
        surface_texture->setSynchronousMode(true);

        mWindowTapIn = new SurfaceTextureClient(surface_texture);
    }

    virtual void setInput(buffer_info_t bufinfo, const char *format) {
        mSurfaceTexture->getST()->setDefaultBufferSize(bufinfo.width, bufinfo.height);
        BufferSourceInput::setInput(bufinfo, format);
#ifndef ANDROID_API_JB_OR_LATER
        mCamera->setBufferSource(mSurfaceTexture->getST(), NULL);
#else
        mCamera->setBufferSource(mSurfaceTexture->getST()->getBufferQueue(), NULL);
#endif
    }

private:
    int mTexId;
    SurfaceTextureBase *mSurfaceTexture;
};

#endif
