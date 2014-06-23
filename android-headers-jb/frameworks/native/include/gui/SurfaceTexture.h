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

#ifndef ANDROID_GUI_SURFACETEXTURE_H
#define ANDROID_GUI_SURFACETEXTURE_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gui/ISurfaceTexture.h>
#include <gui/BufferQueue.h>

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#define ANDROID_GRAPHICS_SURFACETEXTURE_JNI_ID "mSurfaceTexture"

namespace android {
// ----------------------------------------------------------------------------


class String8;

class SurfaceTexture : public virtual RefBase,
        protected BufferQueue::ConsumerListener {
public:
    struct FrameAvailableListener : public virtual RefBase {
        // onFrameAvailable() is called each time an additional frame becomes
        // available for consumption. This means that frames that are queued
        // while in asynchronous mode only trigger the callback if no previous
        // frames are pending. Frames queued while in synchronous mode always
        // trigger the callback.
        //
        // This is called without any lock held and can be called concurrently
        // by multiple threads.
        virtual void onFrameAvailable() = 0;
    };

    // SurfaceTexture constructs a new SurfaceTexture object. tex indicates the
    // name of the OpenGL ES texture to which images are to be streamed.
    // allowSynchronousMode specifies whether or not synchronous mode can be
    // enabled. texTarget specifies the OpenGL ES texture target to which the
    // texture will be bound in updateTexImage. useFenceSync specifies whether
    // fences should be used to synchronize access to buffers if that behavior
    // is enabled at compile-time. A custom bufferQueue can be specified
    // if behavior for queue/dequeue/connect etc needs to be customized.
    // Otherwise a default BufferQueue will be created and used.
    //
    // For legacy reasons, the SurfaceTexture is created in a state where it is
    // considered attached to an OpenGL ES context for the purposes of the
    // attachToContext and detachFromContext methods. However, despite being
    // considered "attached" to a context, the specific OpenGL ES context
    // doesn't get latched until the first call to updateTexImage. After that
    // point, all calls to updateTexImage must be made with the same OpenGL ES
    // context current.
    //
    // A SurfaceTexture may be detached from one OpenGL ES context and then
    // attached to a different context using the detachFromContext and
    // attachToContext methods, respectively. The intention of these methods is
    // purely to allow a SurfaceTexture to be transferred from one consumer
    // context to another. If such a transfer is not needed there is no
    // requirement that either of these methods be called.
    SurfaceTexture(GLuint tex, bool allowSynchronousMode = true,
            GLenum texTarget = GL_TEXTURE_EXTERNAL_OES, bool useFenceSync = true,
            const sp<BufferQueue> &bufferQueue = 0);

    virtual ~SurfaceTexture();

    // updateTexImage sets the image contents of the target texture to that of
    // the most recently queued buffer.
    //
    // This call may only be made while the OpenGL ES context to which the
    // target texture belongs is bound to the calling thread.
    status_t updateTexImage();

    // setBufferCountServer set the buffer count. If the client has requested
    // a buffer count using setBufferCount, the server-buffer count will
    // take effect once the client sets the count back to zero.
    status_t setBufferCountServer(int bufferCount);

    // getTransformMatrix retrieves the 4x4 texture coordinate transform matrix
    // associated with the texture image set by the most recent call to
    // updateTexImage.
    //
    // This transform matrix maps 2D homogeneous texture coordinates of the form
    // (s, t, 0, 1) with s and t in the inclusive range [0, 1] to the texture
    // coordinate that should be used to sample that location from the texture.
    // Sampling the texture outside of the range of this transform is undefined.
    //
    // This transform is necessary to compensate for transforms that the stream
    // content producer may implicitly apply to the content. By forcing users of
    // a SurfaceTexture to apply this transform we avoid performing an extra
    // copy of the data that would be needed to hide the transform from the
    // user.
    //
    // The matrix is stored in column-major order so that it may be passed
    // directly to OpenGL ES via the glLoadMatrixf or glUniformMatrix4fv
    // functions.
    void getTransformMatrix(float mtx[16]);

    // getTimestamp retrieves the timestamp associated with the texture image
    // set by the most recent call to updateTexImage.
    //
    // The timestamp is in nanoseconds, and is monotonically increasing. Its
    // other semantics (zero point, etc) are source-dependent and should be
    // documented by the source.
    int64_t getTimestamp();

    // setFrameAvailableListener sets the listener object that will be notified
    // when a new frame becomes available.
    void setFrameAvailableListener(const sp<FrameAvailableListener>& listener);

    // getAllocator retrieves the binder object that must be referenced as long
    // as the GraphicBuffers dequeued from this SurfaceTexture are referenced.
    // Holding this binder reference prevents SurfaceFlinger from freeing the
    // buffers before the client is done with them.
    sp<IBinder> getAllocator();

    // setDefaultBufferSize is used to set the size of buffers returned by
    // requestBuffers when a with and height of zero is requested.
    // A call to setDefaultBufferSize() may trigger requestBuffers() to
    // be called from the client.
    // The width and height parameters must be no greater than the minimum of
    // GL_MAX_VIEWPORT_DIMS and GL_MAX_TEXTURE_SIZE (see: glGetIntegerv).
    // An error due to invalid dimensions might not be reported until
    // updateTexImage() is called.
    status_t setDefaultBufferSize(uint32_t width, uint32_t height);

    // setFilteringEnabled sets whether the transform matrix should be computed
    // for use with bilinear filtering.
    void setFilteringEnabled(bool enabled);

    // getCurrentBuffer returns the buffer associated with the current image.
    sp<GraphicBuffer> getCurrentBuffer() const;

    // getCurrentTextureTarget returns the texture target of the current
    // texture as returned by updateTexImage().
    GLenum getCurrentTextureTarget() const;

    // getCurrentCrop returns the cropping rectangle of the current buffer.
    Rect getCurrentCrop() const;

    // getCurrentTransform returns the transform of the current buffer.
    uint32_t getCurrentTransform() const;

    // getCurrentScalingMode returns the scaling mode of the current buffer.
    uint32_t getCurrentScalingMode() const;

    // isSynchronousMode returns whether the SurfaceTexture is currently in
    // synchronous mode.
    bool isSynchronousMode() const;

    // abandon frees all the buffers and puts the SurfaceTexture into the
    // 'abandoned' state.  Once put in this state the SurfaceTexture can never
    // leave it.  When in the 'abandoned' state, all methods of the
    // ISurfaceTexture interface will fail with the NO_INIT error.
    //
    // Note that while calling this method causes all the buffers to be freed
    // from the perspective of the the SurfaceTexture, if there are additional
    // references on the buffers (e.g. if a buffer is referenced by a client or
    // by OpenGL ES as a texture) then those buffer will remain allocated.
    void abandon();

    // set the name of the SurfaceTexture that will be used to identify it in
    // log messages.
    void setName(const String8& name);

    // These functions call the corresponding BufferQueue implementation
    // so the refactoring can proceed smoothly
    status_t setDefaultBufferFormat(uint32_t defaultFormat);
    status_t setConsumerUsageBits(uint32_t usage);
    status_t setTransformHint(uint32_t hint);
    virtual status_t setSynchronousMode(bool enabled);

    // getBufferQueue returns the BufferQueue object to which this
    // SurfaceTexture is connected.
    sp<BufferQueue> getBufferQueue() const;

    // detachFromContext detaches the SurfaceTexture from the calling thread's
    // current OpenGL ES context.  This context must be the same as the context
    // that was current for previous calls to updateTexImage.
    //
    // Detaching a SurfaceTexture from an OpenGL ES context will result in the
    // deletion of the OpenGL ES texture object into which the images were being
    // streamed.  After a SurfaceTexture has been detached from the OpenGL ES
    // context calls to updateTexImage will fail returning INVALID_OPERATION
    // until the SurfaceTexture is attached to a new OpenGL ES context using the
    // attachToContext method.
    status_t detachFromContext();

    // attachToContext attaches a SurfaceTexture that is currently in the
    // 'detached' state to the current OpenGL ES context.  A SurfaceTexture is
    // in the 'detached' state iff detachFromContext has successfully been
    // called and no calls to attachToContext have succeeded since the last
    // detachFromContext call.  Calls to attachToContext made on a
    // SurfaceTexture that is not in the 'detached' state will result in an
    // INVALID_OPERATION error.
    //
    // The tex argument specifies the OpenGL ES texture object name in the
    // new context into which the image contents will be streamed.  A successful
    // call to attachToContext will result in this texture object being bound to
    // the texture target and populated with the image contents that were
    // current at the time of the last call to detachFromContext.
    status_t attachToContext(GLuint tex);

    // dump our state in a String
    virtual void dump(String8& result) const;
    virtual void dump(String8& result, const char* prefix, char* buffer, size_t SIZE) const;

protected:

    // Implementation of the BufferQueue::ConsumerListener interface.  These
    // calls are used to notify the SurfaceTexture of asynchronous events in the
    // BufferQueue.
    virtual void onFrameAvailable();
    virtual void onBuffersReleased();

    static bool isExternalFormat(uint32_t format);

private:
    // this version of updateTexImage() takes a functor used to reject or not
    // the newly acquired buffer.
    // this API is TEMPORARY and intended to be used by SurfaceFlinger only,
    // which is why class Layer is made a friend of SurfaceTexture below.
    class BufferRejecter {
        friend class SurfaceTexture;
        virtual bool reject(const sp<GraphicBuffer>& buf,
                const BufferQueue::BufferItem& item) = 0;
    protected:
        virtual ~BufferRejecter() { }
    };
    friend class Layer;
    status_t updateTexImage(BufferRejecter* rejecter);

    // createImage creates a new EGLImage from a GraphicBuffer.
    EGLImageKHR createImage(EGLDisplay dpy,
            const sp<GraphicBuffer>& graphicBuffer);

    // freeBufferLocked frees up the given buffer slot.  If the slot has been
    // initialized this will release the reference to the GraphicBuffer in that
    // slot and destroy the EGLImage in that slot.  Otherwise it has no effect.
    //
    // This method must be called with mMutex locked.
    void freeBufferLocked(int slotIndex);

    // computeCurrentTransformMatrix computes the transform matrix for the
    // current texture.  It uses mCurrentTransform and the current GraphicBuffer
    // to compute this matrix and stores it in mCurrentTransformMatrix.
    void computeCurrentTransformMatrix();

    // syncForReleaseLocked performs the synchronization needed to release the
    // current slot from an OpenGL ES context.  If needed it will set the
    // current slot's fence to guard against a producer accessing the buffer
    // before the outstanding accesses have completed.
    status_t syncForReleaseLocked(EGLDisplay dpy);

    // The default consumer usage flags that SurfaceTexture always sets on its
    // BufferQueue instance; these will be OR:d with any additional flags passed
    // from the SurfaceTexture user. In particular, SurfaceTexture will always
    // consume buffers as hardware textures.
    static const uint32_t DEFAULT_USAGE_FLAGS = GraphicBuffer::USAGE_HW_TEXTURE;

    // mCurrentTextureBuf is the graphic buffer of the current texture. It's
    // possible that this buffer is not associated with any buffer slot, so we
    // must track it separately in order to support the getCurrentBuffer method.
    sp<GraphicBuffer> mCurrentTextureBuf;

    // mCurrentCrop is the crop rectangle that applies to the current texture.
    // It gets set each time updateTexImage is called.
    Rect mCurrentCrop;

    // mCurrentTransform is the transform identifier for the current texture. It
    // gets set each time updateTexImage is called.
    uint32_t mCurrentTransform;

    // mCurrentScalingMode is the scaling mode for the current texture. It gets
    // set to each time updateTexImage is called.
    uint32_t mCurrentScalingMode;

    // mCurrentTransformMatrix is the transform matrix for the current texture.
    // It gets computed by computeTransformMatrix each time updateTexImage is
    // called.
    float mCurrentTransformMatrix[16];

    // mCurrentTimestamp is the timestamp for the current texture. It
    // gets set each time updateTexImage is called.
    int64_t mCurrentTimestamp;

    uint32_t mDefaultWidth, mDefaultHeight;

    // mFilteringEnabled indicates whether the transform matrix is computed for
    // use with bilinear filtering. It defaults to true and is changed by
    // setFilteringEnabled().
    bool mFilteringEnabled;

    // mTexName is the name of the OpenGL texture to which streamed images will
    // be bound when updateTexImage is called. It is set at construction time
    // and can be changed with a call to attachToContext.
    GLuint mTexName;

    // mUseFenceSync indicates whether creation of the EGL_KHR_fence_sync
    // extension should be used to prevent buffers from being dequeued before
    // it's safe for them to be written. It gets set at construction time and
    // never changes.
    const bool mUseFenceSync;

    // mTexTarget is the GL texture target with which the GL texture object is
    // associated.  It is set in the constructor and never changed.  It is
    // almost always GL_TEXTURE_EXTERNAL_OES except for one use case in Android
    // Browser.  In that case it is set to GL_TEXTURE_2D to allow
    // glCopyTexSubImage to read from the texture.  This is a hack to work
    // around a GL driver limitation on the number of FBO attachments, which the
    // browser's tile cache exceeds.
    const GLenum mTexTarget;

    // EGLSlot contains the information and object references that
    // SurfaceTexture maintains about a BufferQueue buffer slot.
    struct EGLSlot {
        EGLSlot()
        : mEglImage(EGL_NO_IMAGE_KHR),
          mFence(EGL_NO_SYNC_KHR) {
        }

        sp<GraphicBuffer> mGraphicBuffer;

        // mEglImage is the EGLImage created from mGraphicBuffer.
        EGLImageKHR mEglImage;

        // mFence is the EGL sync object that must signal before the buffer
        // associated with this buffer slot may be dequeued. It is initialized
        // to EGL_NO_SYNC_KHR when the buffer is created and (optionally, based
        // on a compile-time option) set to a new sync object in updateTexImage.
        EGLSyncKHR mFence;
    };

    // mEglDisplay is the EGLDisplay with which this SurfaceTexture is currently
    // associated.  It is intialized to EGL_NO_DISPLAY and gets set to the
    // current display when updateTexImage is called for the first time and when
    // attachToContext is called.
    EGLDisplay mEglDisplay;

    // mEglContext is the OpenGL ES context with which this SurfaceTexture is
    // currently associated.  It is initialized to EGL_NO_CONTEXT and gets set
    // to the current GL context when updateTexImage is called for the first
    // time and when attachToContext is called.
    EGLContext mEglContext;

    // mEGLSlots stores the buffers that have been allocated by the BufferQueue
    // for each buffer slot.  It is initialized to null pointers, and gets
    // filled in with the result of BufferQueue::acquire when the
    // client dequeues a buffer from a
    // slot that has not yet been used. The buffer allocated to a slot will also
    // be replaced if the requested buffer usage or geometry differs from that
    // of the buffer allocated to a slot.
    EGLSlot mEGLSlots[BufferQueue::NUM_BUFFER_SLOTS];

    // mAbandoned indicates that the BufferQueue will no longer be used to
    // consume images buffers pushed to it using the ISurfaceTexture interface.
    // It is initialized to false, and set to true in the abandon method.  A
    // BufferQueue that has been abandoned will return the NO_INIT error from
    // all ISurfaceTexture methods capable of returning an error.
    bool mAbandoned;

    // mName is a string used to identify the SurfaceTexture in log messages.
    // It can be set by the setName method.
    String8 mName;

    // mFrameAvailableListener is the listener object that will be called when a
    // new frame becomes available. If it is not NULL it will be called from
    // queueBuffer.
    sp<FrameAvailableListener> mFrameAvailableListener;

    // mCurrentTexture is the buffer slot index of the buffer that is currently
    // bound to the OpenGL texture. It is initialized to INVALID_BUFFER_SLOT,
    // indicating that no buffer slot is currently bound to the texture. Note,
    // however, that a value of INVALID_BUFFER_SLOT does not necessarily mean
    // that no buffer is bound to the texture. A call to setBufferCount will
    // reset mCurrentTexture to INVALID_BUFFER_SLOT.
    int mCurrentTexture;

    // The SurfaceTexture has-a BufferQueue and is responsible for creating this object
    // if none is supplied
    sp<BufferQueue> mBufferQueue;

    // mAttached indicates whether the SurfaceTexture is currently attached to
    // an OpenGL ES context.  For legacy reasons, this is initialized to true,
    // indicating that the SurfaceTexture is considered to be attached to
    // whatever context is current at the time of the first updateTexImage call.
    // It is set to false by detachFromContext, and then set to true again by
    // attachToContext.
    bool mAttached;

    // mMutex is the mutex used to prevent concurrent access to the member
    // variables of SurfaceTexture objects. It must be locked whenever the
    // member variables are accessed.
    mutable Mutex mMutex;
};

// ----------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_GUI_SURFACETEXTURE_H
