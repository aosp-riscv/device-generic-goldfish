/*
**
** Copyright 2017, The Android Open Source Project
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

#define LOG_TAG "android_emulator_multidisplay_JNI"
#include <gui/BufferQueue.h>
#include <gui/BufferItemConsumer.h>
#include <gui/Surface.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>

#include <sys/epoll.h>

#include "utils/Log.h"
#include "../../include/qemu_pipe.h"
#include "nativehelper/JNIHelp.h"
#include <nativehelper/ScopedLocalRef.h>
#include "jni.h"
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/android_view_Surface.h"
#include "../../../goldfish-opengl/shared/OpenglCodecCommon/gralloc_cb.h"

#define MAX_DISPLAYS 10

using namespace android;

static int gFd = 0;

struct FrameListener : public ConsumerBase::FrameAvailableListener {
    sp<BufferItemConsumer> mConsumer;
    uint32_t mId;
    uint32_t mCb;
public:
    void onFrameAvailable(const BufferItem& item) override {
        BufferItem bufferItem;
        mConsumer->acquireBuffer(&bufferItem, 0);
        ANativeWindowBuffer* b = bufferItem.mGraphicBuffer->getNativeBuffer();
        if (b && b->handle) {
            cb_handle_t* cb = (cb_handle_t*)(b->handle);
            if (mCb != cb->hostHandle) {
                ALOGI("sent cb %d", cb->hostHandle);
                mCb = cb->hostHandle;
                std::vector<uint32_t> data = {8, mId, mCb};
                WriteFully(gFd, data.data(), data.size() * sizeof(uint32_t));
            }
        }
        else {
            ALOGE("cannot get native buffer handler");
        }
        mConsumer->releaseBuffer(bufferItem);
    }
    void setDefaultBufferSize(uint32_t w, uint32_t h) {
        mConsumer->setDefaultBufferSize(w, h);
    }
    FrameListener(sp<BufferItemConsumer>& consumer, uint32_t id)
        : mConsumer(consumer), mId(id), mCb(0) { }
};

sp<FrameListener> gFrameListener[MAX_DISPLAYS + 1];

static jobject nativeCreateSurface(JNIEnv *env, jobject obj, jint id, jint width, jint height)
{
    ALOGI("create surface for %d", id);
    // Create surface for this new display
    sp<IGraphicBufferProducer> producer;
    sp<IGraphicBufferConsumer> consumer;
    sp<BufferItemConsumer> bufferItemConsumer;
    BufferQueue::createBufferQueue(&producer, &consumer);
    bufferItemConsumer = new BufferItemConsumer(consumer, GRALLOC_USAGE_HW_RENDER);
    gFrameListener[id] = new FrameListener(bufferItemConsumer, id);
    gFrameListener[id]->setDefaultBufferSize(width, height);
    bufferItemConsumer->setFrameAvailableListener(gFrameListener[id]);
    return android_view_Surface_createFromIGraphicBufferProducer(env, producer);
}

static jint nativeOpen(JNIEnv* env, jobject obj) {
    // Open pipe
    gFd = qemu_pipe_open("multidisplay");
    if (gFd < 0) {
        ALOGE("Error opening multidisplay pipe %d", gFd);
    } else {
        ALOGI("multidisplay pipe connected!!!");
    }
    return gFd;
}

static bool nativeReadPipe(JNIEnv* env, jobject obj, jintArray arr) {
    static const uint8_t ADD = 1;
    static const uint8_t DEL = 2;
    int* arrp = env->GetIntArrayElements(arr, 0);
    uint32_t length;
    ReadFully(gFd, &length, sizeof(length));
    std::vector<uint8_t> args(length, 0);
    ReadFully(gFd, args.data(), (size_t)length);
    switch(args[0]) {
        case ADD: {
            ALOGV("received add event");
            *arrp = ADD;
            for (int i = 1; i < 6; i++) {
                *(arrp + i) = *(uint32_t*)(&args[(i - 1) * 4 + 1]);
            }
            env->ReleaseIntArrayElements(arr, arrp, JNI_COMMIT);
            break;
        }
        case DEL: {
            ALOGV("received del event");
            *arrp = DEL;
            *(arrp + 1) = *(uint32_t*)(&args[1]);
            env->ReleaseIntArrayElements(arr, arrp, JNI_COMMIT);
            break;
        }
        default: {
            ALOGE("unexpected event %d", args[0]);
            return false;
        }
    }
    return true;
}

static bool nativeReleaseListener(JNIEnv* env, jobject obj, jint id) {
    if (gFrameListener[id].get()) {
        ALOGV("clear gFrameListener %d", id);
        gFrameListener[id].clear();
        gFrameListener[id] = nullptr;
    }
    return true;
}

static bool nativeResizeListener(JNIEnv* env, jobject obj, jint id, jint w, jint h) {
    if (gFrameListener[id]) {
        gFrameListener[id]->setDefaultBufferSize(w, h);
        return true;
    }
    return false;
}

static JNINativeMethod sMethods[] = {
    { "nativeOpen", "()I", (void*) nativeOpen },
    { "nativeCreateSurface", "(III)Landroid/view/Surface;", (void*) nativeCreateSurface },
    { "nativeReadPipe", "([I)Z", (void*) nativeReadPipe},
    { "nativeReleaseListener", "(I)Z", (void*) nativeReleaseListener},
    { "nativeResizeListener", "(III)Z", (void*) nativeResizeListener},
};

/*
 * JNI Initialization
 */
jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    JNIEnv *env;

    // Check JNI version
    if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6)) {
        ALOGE("JNI version mismatch error");
        return JNI_ERR;
    }

    jniRegisterNativeMethods(env, "com/android/emulator/multidisplay/MultiDisplayService",
                                    sMethods, NELEM(sMethods));

    return JNI_VERSION_1_6;
}
