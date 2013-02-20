/*
 * Copyright (C) 2012 The Android Open Source Project
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

// FIXME: add well-defined names for cameras

#ifndef ANDROID_INCLUDE_CAMERA_COMMON_H
#define ANDROID_INCLUDE_CAMERA_COMMON_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <cutils/native_handle.h>
#include <system/camera.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

__BEGIN_DECLS

/**
 * The id of this module
 */
#define CAMERA_HARDWARE_MODULE_ID "camera"

/**
 * Module versioning information for the Camera hardware module, based on
 * camera_module_t.common.module_api_version. The two most significant hex
 * digits represent the major version, and the two least significant represent
 * the minor version.
 *
 *******************************************************************************
 * Versions: 0.X - 1.X [CAMERA_MODULE_API_VERSION_1_0]
 *
 *   Camera modules that report these version numbers implement the initial
 *   camera module HAL interface. All camera devices openable through this
 *   module support only version 1 of the camera device HAL. The device_version
 *   and static_camera_characteristics fields of camera_info are not valid. Only
 *   the android.hardware.Camera API can be supported by this module and its
 *   devices.
 *
 *******************************************************************************
 * Version: 2.0 [CAMERA_MODULE_API_VERSION_2_0]
 *
 *   Camera modules that report this version number implement the second version
 *   of the camera module HAL interface. Camera devices openable through this
 *   module may support either version 1.0 or version 2.0 of the camera device
 *   HAL interface. The device_version field of camera_info is always valid; the
 *   static_camera_characteristics field of camera_info is valid if the
 *   device_version field is 2.0 or higher.
 */

/**
 * Predefined macros for currently-defined version numbers
 */

/**
 * All module versions <= HARDWARE_MODULE_API_VERSION(1, 0xFF) must be treated
 * as CAMERA_MODULE_API_VERSION_1_0
 */
#define CAMERA_MODULE_API_VERSION_1_0 HARDWARE_MODULE_API_VERSION(1, 0)
#define CAMERA_MODULE_API_VERSION_2_0 HARDWARE_MODULE_API_VERSION(2, 0)

#define CAMERA_MODULE_API_VERSION_CURRENT CAMERA_MODULE_API_VERSION_2_0

/**
 * All device versions <= HARDWARE_DEVICE_API_VERSION(1, 0xFF) must be treated
 * as CAMERA_DEVICE_API_VERSION_1_0
 */
#define CAMERA_DEVICE_API_VERSION_1_0 HARDWARE_DEVICE_API_VERSION(1, 0)
#define CAMERA_DEVICE_API_VERSION_2_0 HARDWARE_DEVICE_API_VERSION(2, 0)

// Device version 2.0 is experimental
#define CAMERA_DEVICE_API_VERSION_CURRENT CAMERA_DEVICE_API_VERSION_1_0

/**
 * Defined in /system/media/camera/include/system/camera_metadata.h
 */
typedef struct camera_metadata camera_metadata_t;

struct camera_info {
    /**
     * The direction that the camera faces to. It should be CAMERA_FACING_BACK
     * or CAMERA_FACING_FRONT.
     *
     * Version information:
     *   Valid in all camera_module versions
     */
    int facing;

    /**
     * The orientation of the camera image. The value is the angle that the
     * camera image needs to be rotated clockwise so it shows correctly on the
     * display in its natural orientation. It should be 0, 90, 180, or 270.
     *
     * For example, suppose a device has a naturally tall screen. The
     * back-facing camera sensor is mounted in landscape. You are looking at
     * the screen. If the top side of the camera sensor is aligned with the
     * right edge of the screen in natural orientation, the value should be
     * 90. If the top side of a front-facing camera sensor is aligned with the
     * right of the screen, the value should be 270.
     *
     * Version information:
     *   Valid in all camera_module versions
     */
    int orientation;

    /**
     * The value of camera_device_t.common.version.
     *
     * Version information (based on camera_module_t.common.module_api_version):
     *
     *  CAMERA_MODULE_API_VERSION_1_0:
     *
     *    Not valid. Can be assumed to be CAMERA_DEVICE_API_VERSION_1_0. Do
     *    not read this field.
     *
     *  CAMERA_MODULE_API_VERSION_2_0:
     *
     *    Always valid
     *
     */
    uint32_t device_version;

    /**
     * The camera's fixed characteristics, which include all camera metadata in
     * the android.*.info.* sections.
     *
     * Version information (based on camera_module_t.common.module_api_version):
     *
     *  CAMERA_MODULE_API_VERSION_1_0:
     *
     *    Not valid. Extra characteristics are not available. Do not read this
     *    field.
     *
     *  CAMERA_MODULE_API_VERSION_2_0:
     *
     *    Valid if device_version >= CAMERA_DEVICE_API_VERSION_2_0. Do not read
     *    otherwise.
     *
     */
    camera_metadata_t *static_camera_characteristics;
};

typedef struct camera_module {
    hw_module_t common;
    int (*get_number_of_cameras)(void);
    int (*get_camera_info)(int camera_id, struct camera_info *info);
} camera_module_t;

__END_DECLS

#endif /* ANDROID_INCLUDE_CAMERA_COMMON_H */
