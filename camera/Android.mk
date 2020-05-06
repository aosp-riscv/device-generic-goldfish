# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# Emulator camera module########################################################

emulator_camera_module_relative_path := hw
emulator_camera_cflags := -fno-short-enums -DQEMU_HARDWARE
emulator_camera_cflags += -Wno-unused-parameter -Wno-missing-field-initializers
emulator_camera_clang_flags := -Wno-c++11-narrowing
emulator_camera_shared_libraries := \
    libbinder \
    libexif \
    liblog \
    libutils \
    libcutils \
    libEGL \
    libGLESv1_CM \
    libGLESv2 \
    libui \
    libdl \
    libjpeg \
    libcamera_metadata \
    libui \
    android.hardware.graphics.mapper@2.0 \
    libqemupipe.ranchu

emulator_camera_static_libraries := \
	libqemud.ranchu \
	android.hardware.camera.common@1.0-helper \
	libyuv_static

emulator_camera_header_libraries := \
	libgralloc_cb.ranchu

emulator_camera_c_includes := external/libjpeg-turbo \
	external/libexif \
	external/libyuv/files/include \
	frameworks/native/include/media/hardware \
	$(call include-path-for, camera)

emulator_camera_src := \
	EmulatedCameraHal.cpp \
	EmulatedCameraFactory.cpp \
	EmulatedCameraHotplugThread.cpp \
	EmulatedBaseCamera.cpp \
	EmulatedCamera.cpp \
		EmulatedCameraDevice.cpp \
		EmulatedQemuCamera.cpp \
		EmulatedQemuCameraDevice.cpp \
		EmulatedFakeCamera.cpp \
		EmulatedFakeCameraDevice.cpp \
		EmulatedFakeRotatingCameraDevice.cpp \
		Converters.cpp \
		PreviewWindow.cpp \
		CallbackNotifier.cpp \
		QemuClient.cpp \
		JpegCompressor.cpp \
	EmulatedCamera2.cpp \
		EmulatedFakeCamera2.cpp \
		EmulatedQemuCamera2.cpp \
		fake-pipeline2/Scene.cpp \
		fake-pipeline2/Sensor.cpp \
		fake-pipeline2/JpegCompressor.cpp \
	EmulatedCamera3.cpp \
		EmulatedFakeCamera3.cpp \
		EmulatedQemuCamera3.cpp \
		qemu-pipeline3/QemuSensor.cpp \
	Exif.cpp \
	Thumbnail.cpp \
	WorkerThread.cpp \

# Emulator camera - ranchu build################################################

include ${CLEAR_VARS}

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := ${emulator_camera_module_relative_path}
LOCAL_CFLAGS := ${emulator_camera_cflags}
LOCAL_CLANG_CFLAGS += ${emulator_camera_clang_flags}

LOCAL_SHARED_LIBRARIES := ${emulator_camera_shared_libraries}
LOCAL_STATIC_LIBRARIES := ${emulator_camera_static_libraries}
LOCAL_HEADER_LIBRARIES := ${emulator_camera_header_libraries}
LOCAL_C_INCLUDES += ${emulator_camera_c_includes}
LOCAL_SRC_FILES := ${emulator_camera_src}

LOCAL_MODULE := camera.ranchu

# Symlink media profile configurations from /vendor/etc to /data/vendor/etc/
include $(BUILD_SHARED_LIBRARY)

# Emulator camera - test binary################################################

include ${CLEAR_VARS}

LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := ${emulator_camera_module_relative_path}
LOCAL_CFLAGS := ${emulator_camera_cflags}
LOCAL_CLANG_CFLAGS += ${emulator_camera_clang_flags}

LOCAL_SHARED_LIBRARIES := ${emulator_camera_shared_libraries}
LOCAL_STATIC_LIBRARIES := ${emulator_camera_static_libraries}
LOCAL_HEADER_LIBRARIES := ${emulator_camera_header_libraries}
LOCAL_C_INCLUDES += ${emulator_camera_c_includes}
LOCAL_SRC_FILES := ${emulator_camera_src}
LOCAL_SRC_FILES += EmulatorCameraTest.cpp

LOCAL_MODULE := emulatorcameratest
include $(BUILD_EXECUTABLE)