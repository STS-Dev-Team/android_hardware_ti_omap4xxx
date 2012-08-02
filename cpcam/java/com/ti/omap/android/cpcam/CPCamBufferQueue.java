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


package com.ti.omap.android.cpcam;

import java.lang.ref.WeakReference;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

/**
 * BufferQueue provides image and video buffers for the for the system to use.
 * These buffers can be accessed by the application as well as the lower level
 * system and drivers in a zero-copy fashion.
 *
 * This implementation is based off of SurfaceTexture.java found in
 * frameworks/base project
 */
public class CPCamBufferQueue {

    private EventHandler mEventHandler;
    private OnFrameAvailableListener mOnFrameAvailableListener;

    /**
     * This field is used by native code, do not access or modify.
     */
    private int mBufferQueue;
    private int mNativeContext;

    /**
     * Callback interface for being notified that a new stream frame is available.
     */
    public interface OnFrameAvailableListener {
        void onFrameAvailable(CPCamBufferQueue bufferQueue);
    }

    /**
     * Exception thrown when a surface couldn't be created or resized
     */
    public static class OutOfResourcesException extends Exception {
        public OutOfResourcesException() {
        }
        public OutOfResourcesException(String name) {
            super(name);
        }
    }

    /**
     * Construct a new BufferQueue.
     *
     */
    public CPCamBufferQueue() {
        this(false);
    }

    /**
     * Construct a new BufferQueue.
     *
     * @param allowSynchronousMode whether the BufferQueue can run in the synchronous mode.
     *      To avoid the thread block, set allowSynchronousMode to false.
     *
     */
    public CPCamBufferQueue(boolean allowSynchronousMode) {
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(looper);
        } else {
            mEventHandler = null;
        }
        nativeInit(new WeakReference<CPCamBufferQueue>(this), allowSynchronousMode);
    }

    /**
     * Register a callback to be invoked when a new image frame becomes available to the
     * BufferQueue.
     */
    public void setOnFrameAvailableListener(OnFrameAvailableListener l) {
        mOnFrameAvailableListener = l;
    }

    /**
     * Set the default size of the image buffers.  The image producer may override the buffer size,
     * in which case the producer-set buffer size will be used, not the default size set by this
     * method.  Both video and camera based image producers do override the size.
     */
    public void setDefaultBufferSize(int width, int height) {
        nativeSetDefaultBufferSize(width, height);
    }

    /**
     * Updates and takes a reference to the current buffer from the queue.
     *
     * Returns buffer slot index of the buffer
     *
     * Applications must call {@link #releaseBuffer} to release reference to
     * buffer the buffer
     */
    public int acquireBuffer() {
        return nativeAcquireBuffer();
    }

    /**
     * Releases reference to buffer.
     *
     * @param slot indicates the slot index of the buffer to be released
     *
     */
    public void releaseBuffer(int slot) {
        nativeReleaseBuffer(slot);
    }

    /**
     * Retrieve the timestamp associated with the texture image set by the most recent call to
     * updateTexImage.
     *
     * This timestamp is in nanoseconds, and is normally monotonically increasing. The timestamp
     * should be unaffected by time-of-day adjustments, and for a camera should be strictly
     * monotonic but for a MediaPlayer may be reset when the position is set.  The
     * specific meaning and zero point of the timestamp depends on the source providing images to
     * the CPCamBufferQueue. Unless otherwise specified by the image source, timestamps cannot
     * generally be compared across CPCamBufferQueue instances, or across multiple program
     * invocations. It is mostly useful for determining time offsets between subsequent frames.
     */

    public long getTimestamp(int slot) {
        return nativeGetTimestamp(slot);
    }

    /**
     * release() frees all the buffers and puts the BufferQueue into the
     * 'abandoned' state. Once put in this state the BufferQueue can never
     * leave it. When in the 'abandoned' state, all methods of the
     * interface will fail with the NO_INIT error.
     *
     * Note that while calling this method causes all the buffers to be freed
     * from the perspective of the the BufferQueue, if there are additional
     * references on the buffers (e.g. if a buffer is referenced by a client)
     * then those buffer will remain allocated.
     *
     * Always call this method when you are done with BufferQueue. Failing
     * to do so may delay resource deallocation for a significant amount of
     * time.
     */
    public void release() {
        nativeRelease();
    }

    protected void finalize() throws Throwable {
        try {
            nativeFinalize();
        } finally {
            super.finalize();
        }
    }

    private class EventHandler extends Handler {
        public EventHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (mOnFrameAvailableListener != null) {
                mOnFrameAvailableListener.onFrameAvailable(CPCamBufferQueue.this);
            }
        }
    }

    /**
     * This method is invoked from native code only.
     */
    @SuppressWarnings({"UnusedDeclaration"})
    private static void postEventFromNative(Object selfRef) {
        WeakReference weakSelf = (WeakReference)selfRef;
        CPCamBufferQueue st = (CPCamBufferQueue)weakSelf.get();
        if (st == null) {
            return;
        }

        if (st.mEventHandler != null) {
            Message m = st.mEventHandler.obtainMessage();
            st.mEventHandler.sendMessage(m);
        }
    }

    private native void nativeInit(Object weakSelf, boolean allowSynchronousMode);
    private native void nativeFinalize();
    private native long nativeGetTimestamp(int slot);
    private native void nativeSetDefaultBufferSize(int width, int height);
    private native int nativeAcquireBuffer();
    private native void nativeReleaseBuffer(int slot);
    private native int nativeGetQueuedCount();
    private native void nativeRelease();

    /*
     * We use a class initializer to allow the native code to cache some
     * field offsets.
     */
    private static native void nativeClassInit();
    static { nativeClassInit(); }
}
