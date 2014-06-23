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

#ifndef ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H
#define ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H

#include <stdint.h>
#include <sys/cdefs.h>

#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <cutils/native_handle.h>

#include <hardware/hwcomposer_defs.h>

__BEGIN_DECLS

/*****************************************************************************/

// for compatibility
#define HWC_MODULE_API_VERSION      HWC_MODULE_API_VERSION_0_1
#define HWC_DEVICE_API_VERSION      HWC_DEVICE_API_VERSION_0_1
#define HWC_API_VERSION             HWC_DEVICE_API_VERSION

/**
 * The id of this module
 */
#define HWC_HARDWARE_MODULE_ID "hwcomposer"

/**
 * Name of the sensors device to open
 */
#define HWC_HARDWARE_COMPOSER   "composer"


struct hwc_composer_device;

/*
 * availability: HWC_DEVICE_API_VERSION_0_3
 *
 * struct hwc_methods cannot be embedded in other structures as
 * sizeof(struct hwc_methods) cannot be relied upon.
 *
 */
typedef struct hwc_methods {

    /*************************************************************************
     * HWC_DEVICE_API_VERSION_0_3
     *************************************************************************/

    /*
     * eventControl(..., event, enabled)
     * Enables or disables h/w composer events.
     *
     * eventControl can be called from any thread and takes effect
     * immediately.
     *
     *  Supported events are:
     *      HWC_EVENT_VSYNC
     *
     * returns -EINVAL if the "event" parameter is not one of the value above
     * or if the "enabled" parameter is not 0 or 1.
     */

    int (*eventControl)(
            struct hwc_composer_device* dev, int event, int enabled);

} hwc_methods_t;

typedef struct hwc_rect {
    int left;
    int top;
    int right;
    int bottom;
} hwc_rect_t;

typedef struct hwc_region {
    size_t numRects;
    hwc_rect_t const* rects;
} hwc_region_t;

typedef struct hwc_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} hwc_color_t;

typedef struct hwc_layer {
    /*
     * initially set to HWC_FRAMEBUFFER or HWC_BACKGROUND.
     * HWC_FRAMEBUFFER
     *   indicates the layer will be drawn into the framebuffer
     *   using OpenGL ES.
     *   The HWC can toggle this value to HWC_OVERLAY, to indicate
     *   it will handle the layer.
     *
     * HWC_BACKGROUND
     *   indicates this is a special "background"  layer. The only valid
     *   field is backgroundColor. HWC_BACKGROUND can only be used with
     *   HWC_API_VERSION >= 0.2
     *   The HWC can toggle this value to HWC_FRAMEBUFFER, to indicate
     *   it CANNOT handle the background color
     *
     */
    int32_t compositionType;

    /* see hwc_layer_t::hints above */
    uint32_t hints;

    /* see hwc_layer_t::flags above */
    uint32_t flags;

    union {
        /* color of the background.  hwc_color_t.a is ignored */
        hwc_color_t backgroundColor;

        struct {
            /* handle of buffer to compose. This handle is guaranteed to have been
             * allocated from gralloc using the GRALLOC_USAGE_HW_COMPOSER usage flag. If
             * the layer's handle is unchanged across two consecutive prepare calls and
             * the HWC_GEOMETRY_CHANGED flag is not set for the second call then the
             * HWComposer implementation may assume that the contents of the buffer have
             * not changed. */
            buffer_handle_t handle;

            /* transformation to apply to the buffer during composition */
            uint32_t transform;

            /* blending to apply during composition */
            int32_t blending;

            /* area of the source to consider, the origin is the top-left corner of
             * the buffer */
            hwc_rect_t sourceCrop;

            /* where to composite the sourceCrop onto the display. The sourceCrop
             * is scaled using linear filtering to the displayFrame. The origin is the
             * top-left corner of the screen.
             */
            hwc_rect_t displayFrame;

            /* visible region in screen space. The origin is the
             * top-left corner of the screen.
             * The visible region INCLUDES areas overlapped by a translucent layer.
             */
            hwc_region_t visibleRegionScreen;
        };
    };
} hwc_layer_t;


/*
 * hwc_layer_list_t::flags values
 */
enum {
    /*
     * HWC_GEOMETRY_CHANGED is set by SurfaceFlinger to indicate that the list
     * passed to (*prepare)() has changed by more than just the buffer handles.
     */
    HWC_GEOMETRY_CHANGED = 0x00000001,
};

/*
 * List of layers.
 * The handle members of hwLayers elements must be unique.
 */
typedef struct hwc_layer_list {
    uint32_t flags;
    size_t numHwLayers;
    hwc_layer_t hwLayers[0];
} hwc_layer_list_t;

/* This represents a display, typically an EGLDisplay object */
typedef void* hwc_display_t;

/* This represents a surface, typically an EGLSurface object  */
typedef void* hwc_surface_t;


/* see hwc_composer_device::registerProcs()
 * Any of the callbacks can be NULL, in which case the corresponding
 * functionality is not supported.
 */
typedef struct hwc_procs {
    /*
     * (*invalidate)() triggers a screen refresh, in particular prepare and set
     * will be called shortly after this call is made. Note that there is
     * NO GUARANTEE that the screen refresh will happen after invalidate()
     * returns (in particular, it could happen before).
     * invalidate() is GUARANTEED TO NOT CALL BACK into the h/w composer HAL and
     * it is safe to call invalidate() from any of hwc_composer_device
     * hooks, unless noted otherwise.
     */
    void (*invalidate)(struct hwc_procs* procs);

    /*
     * (*vsync)() is called by the h/w composer HAL when a vsync event is
     * received and HWC_EVENT_VSYNC is enabled (see: hwc_event_control).
     *
     * the "zero" parameter must always be 0.
     * the "timestamp" parameter is the system monotonic clock timestamp in
     *   nanosecond of when the vsync event happened.
     *
     * vsync() is GUARANTEED TO NOT CALL BACK into the h/w composer HAL.
     *
     * It is expected that vsync() is called from a thread of at least
     * HAL_PRIORITY_URGENT_DISPLAY with as little latency as possible,
     * typically less than 0.5 ms.
     *
     * It is a (silent) error to have HWC_EVENT_VSYNC enabled when calling
     * hwc_composer_device.set(..., 0, 0, 0) (screen off). The implementation
     * can either stop or continue to process VSYNC events, but must not
     * crash or cause other problems.
     *
     */
    void (*vsync)(struct hwc_procs* procs, int zero, int64_t timestamp);
} hwc_procs_t;


/*****************************************************************************/

typedef struct hwc_module {
    struct hw_module_t common;
} hwc_module_t;


typedef struct hwc_composer_device {
    struct hw_device_t common;

    /*
     * (*prepare)() is called for each frame before composition and is used by
     * SurfaceFlinger to determine what composition steps the HWC can handle.
     *
     * (*prepare)() can be called more than once, the last call prevails.
     *
     * The HWC responds by setting the compositionType field to either
     * HWC_FRAMEBUFFER or HWC_OVERLAY. In the former case, the composition for
     * this layer is handled by SurfaceFlinger with OpenGL ES, in the later
     * case, the HWC will have to handle this layer's composition.
     *
     * (*prepare)() is called with HWC_GEOMETRY_CHANGED to indicate that the
     * list's geometry has changed, that is, when more than just the buffer's
     * handles have been updated. Typically this happens (but is not limited to)
     * when a window is added, removed, resized or moved.
     *
     * a NULL list parameter or a numHwLayers of zero indicates that the
     * entire composition will be handled by SurfaceFlinger with OpenGL ES.
     *
     * returns: 0 on success. An negative error code on error. If an error is
     * returned, SurfaceFlinger will assume that none of the layer will be
     * handled by the HWC.
     */
    int (*prepare)(struct hwc_composer_device *dev, hwc_layer_list_t* list);


    /*
     * (*set)() is used in place of eglSwapBuffers(), and assumes the same
     * functionality, except it also commits the work list atomically with
     * the actual eglSwapBuffers().
     *
     * The list parameter is guaranteed to be the same as the one returned
     * from the last call to (*prepare)().
     *
     * When this call returns the caller assumes that:
     *
     * - the display will be updated in the near future with the content
     *   of the work list, without artifacts during the transition from the
     *   previous frame.
     *
     * - all objects are available for immediate access or destruction, in
     *   particular, hwc_region_t::rects data and hwc_layer_t::layer's buffer.
     *   Note that this means that immediately accessing (potentially from a
     *   different process) a buffer used in this call will not result in
     *   screen corruption, the driver must apply proper synchronization or
     *   scheduling (eg: block the caller, such as gralloc_module_t::lock(),
     *   OpenGL ES, Camera, Codecs, etc..., or schedule the caller's work
     *   after the buffer is freed from the actual composition).
     *
     * a NULL list parameter or a numHwLayers of zero indicates that the
     * entire composition has been handled by SurfaceFlinger with OpenGL ES.
     * In this case, (*set)() behaves just like eglSwapBuffers().
     *
     * dpy, sur, and list are set to NULL to indicate that the screen is
     * turning off. This happens WITHOUT prepare() being called first.
     * This is a good time to free h/w resources and/or power
     * the relevant h/w blocks down.
     *
     * IMPORTANT NOTE: there is an implicit layer containing opaque black
     * pixels behind all the layers in the list.
     * It is the responsibility of the hwcomposer module to make
     * sure black pixels are output (or blended from).
     *
     * returns: 0 on success. An negative error code on error:
     *    HWC_EGL_ERROR: eglGetError() will provide the proper error code
     *    Another code for non EGL errors.
     *
     */
    int (*set)(struct hwc_composer_device *dev,
                hwc_display_t dpy,
                hwc_surface_t sur,
                hwc_layer_list_t* list);
    /*
     * This field is OPTIONAL and can be NULL.
     *
     * If non NULL it will be called by SurfaceFlinger on dumpsys
     */
    void (*dump)(struct hwc_composer_device* dev, char *buff, int buff_len);

    /*
     * This field is OPTIONAL and can be NULL.
     *
     * (*registerProcs)() registers a set of callbacks the h/w composer HAL
     * can later use. It is FORBIDDEN to call any of the callbacks from
     * within registerProcs(). registerProcs() must save the hwc_procs_t pointer
     * which is needed when calling a registered callback.
     * Each call to registerProcs replaces the previous set of callbacks.
     * registerProcs is called with NULL to unregister all callbacks.
     *
     * Any of the callbacks can be NULL, in which case the corresponding
     * functionality is not supported.
     */
    void (*registerProcs)(struct hwc_composer_device* dev,
            hwc_procs_t const* procs);

    /*
     * This field is OPTIONAL and can be NULL.
     * availability: HWC_DEVICE_API_VERSION_0_2
     *
     * Used to retrieve information about the h/w composer
     *
     * Returns 0 on success or -errno on error.
     */
    int (*query)(struct hwc_composer_device* dev, int what, int* value);

    /*
     * Reserved for future use. Must be NULL.
     */
    void* reserved_proc[4];

    /*
     * This field is OPTIONAL and can be NULL.
     * availability: HWC_DEVICE_API_VERSION_0_3
     */
    hwc_methods_t const *methods;

} hwc_composer_device_t;


/** convenience API for opening and closing a device */

static inline int hwc_open(const struct hw_module_t* module,
        hwc_composer_device_t** device) {
    return module->methods->open(module,
            HWC_HARDWARE_COMPOSER, (struct hw_device_t**)device);
}

static inline int hwc_close(hwc_composer_device_t* device) {
    return device->common.close(&device->common);
}


/*****************************************************************************/

__END_DECLS

#endif /* ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H */
