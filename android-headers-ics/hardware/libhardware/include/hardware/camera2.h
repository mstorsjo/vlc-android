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

#ifndef ANDROID_INCLUDE_CAMERA2_H
#define ANDROID_INCLUDE_CAMERA2_H

#include "camera_common.h"

/**
 * Camera device HAL 2.0 [ CAMERA_DEVICE_API_VERSION_2_0 ]
 *
 * EXPERIMENTAL.
 *
 * Supports both the android.hardware.ProCamera and
 * android.hardware.Camera APIs.
 *
 * Camera devices that support this version of the HAL must return
 * CAMERA_DEVICE_API_VERSION_2_0 in camera_device_t.common.version and in
 * camera_info_t.device_version (from camera_module_t.get_camera_info).
 *
 * Camera modules that may contain version 2.0 devices must implement at least
 * version 2.0 of the camera module interface (as defined by
 * camera_module_t.common.module_api_version).
 *
 * See camera_common.h for more details.
 *
 */

__BEGIN_DECLS

struct camera2_device;

/**
 * Output image stream queue management
 */

typedef struct camera2_stream_ops {
    int (*dequeue_buffer)(struct camera2_stream_ops* w,
                          buffer_handle_t** buffer, int *stride);
    int (*enqueue_buffer)(struct camera2_stream_ops* w,
                buffer_handle_t* buffer);
    int (*cancel_buffer)(struct camera2_stream_ops* w,
                buffer_handle_t* buffer);
    int (*set_buffer_count)(struct camera2_stream_ops* w, int count);
    int (*set_buffers_geometry)(struct camera2_stream_ops* pw,
                int w, int h, int format);
    int (*set_crop)(struct camera2_stream_ops *w,
                int left, int top, int right, int bottom);
    // Timestamps are measured in nanoseconds, and must be comparable
    // and monotonically increasing between two frames in the same
    // preview stream. They do not need to be comparable between
    // consecutive or parallel preview streams, cameras, or app runs.
    // The timestamp must be the time at the start of image exposure.
    int (*set_timestamp)(struct camera2_stream_ops *w, int64_t timestamp);
    int (*set_usage)(struct camera2_stream_ops* w, int usage);
    int (*get_min_undequeued_buffer_count)(const struct camera2_stream_ops *w,
                int *count);
    int (*lock_buffer)(struct camera2_stream_ops* w,
                buffer_handle_t* buffer);
} camera2_stream_ops_t;

/**
 * Metadata queue management, used for requests sent to HAL module, and for
 * frames produced by the HAL.
 *
 * Queue protocol:
 *
 * The source holds the queue and its contents. At start, the queue is empty.
 *
 * 1. When the first metadata buffer is placed into the queue, the source must
 *    signal the destination by calling notify_queue_not_empty().
 *
 * 2. After receiving notify_queue_not_empty, the destination must call
 *    dequeue() once it's ready to handle the next buffer.
 *
 * 3. Once the destination has processed a buffer, it should try to dequeue
 *    another buffer. If there are no more buffers available, dequeue() will
 *    return NULL. In this case, when a buffer becomes available, the source
 *    must call notify_queue_not_empty() again. If the destination receives a
 *    NULL return from dequeue, it does not need to query the queue again until
 *    a notify_queue_not_empty() call is received from the source.
 *
 * 4. If the destination calls buffer_count() and receives 0, this does not mean
 *    that the source will provide a notify_queue_not_empty() call. The source
 *    must only provide such a call after the destination has received a NULL
 *    from dequeue, or on initial startup.
 *
 * 5. The dequeue() call in response to notify_queue_not_empty() may be on the
 *    same thread as the notify_queue_not_empty() call. The source must not
 *    deadlock in that case.
 */

typedef struct camera2_metadata_queue_src_ops {
    /**
     * Get count of buffers in queue
     */
    int (*buffer_count)(camera2_metadata_queue_src_ops *q);

    /**
     * Get a metadata buffer from the source. Returns OK if a request is
     * available, placing a pointer to it in next_request.
     */
    int (*dequeue)(camera2_metadata_queue_src_ops *q,
            camera_metadata_t **buffer);
    /**
     * Return a metadata buffer to the source once it has been used
     */
    int (*free)(camera2_metadata_queue_src_ops *q,
            camera_metadata_t *old_buffer);

} camera2_metadata_queue_src_ops_t;

typedef struct camera2_metadata_queue_dst_ops {
    /**
     * Notify destination that the queue is no longer empty
     */
    int (*notify_queue_not_empty)(struct camera2_metadata_queue_dst_ops *);

} camera2_metadata_queue_dst_ops_t;

/* Defined in camera_metadata.h */
typedef struct vendor_tag_query_ops vendor_tag_query_ops_t;

/**
 * Asynchronous notification callback from the HAL, fired for various
 * reasons. Only for information independent of frame capture, or that require
 * specific timing.
 */
typedef void (*camera2_notify_callback)(int32_t msg_type,
        int32_t ext1,
        int32_t ext2,
        void *user);

/**
 * Possible message types for camera2_notify_callback
 */
enum {
    /**
     * A serious error has occurred. Argument ext1 contains the error code, and
     * ext2 and user contain any error-specific information.
     */
    CAMERA2_MSG_ERROR   = 0x0001,
    /**
     * The exposure of a given request has begun. Argument ext1 contains the
     * request id.
     */
    CAMERA2_MSG_SHUTTER = 0x0002
};

/**
 * Error codes for CAMERA_MSG_ERROR
 */
enum {
    /**
     * A serious failure occured. Camera device may not work without reboot, and
     * no further frames or buffer streams will be produced by the
     * device. Device should be treated as closed.
     */
    CAMERA2_MSG_ERROR_HARDWARE_FAULT = 0x0001,
    /**
     * A serious failure occured. No further frames or buffer streams will be
     * produced by the device. Device should be treated as closed. The client
     * must reopen the device to use it again.
     */
    CAMERA2_MSG_ERROR_DEVICE_FAULT =   0x0002,
    /**
     * The camera service has failed. Device should be treated as released. The client
     * must reopen the device to use it again.
     */
    CAMERA2_MSG_ERROR_SERVER_FAULT =   0x0003
};

typedef struct camera2_device_ops {
    /**
     * Input request queue methods
     */
    int (*set_request_queue_src_ops)(struct camera2_device *,
            camera2_metadata_queue_src_ops *queue_src_ops);

    int (*get_request_queue_dst_ops)(struct camera2_device *,
            camera2_metadata_queue_dst_ops **queue_dst_ops);

    /**
     * Input reprocessing queue methods
     */
    int (*set_reprocess_queue_ops)(struct camera2_device *,
            camera2_metadata_queue_src_ops *queue_src_ops);

    int (*get_reprocess_queue_dst_ops)(struct camera2_device *,
            camera2_metadata_queue_dst_ops **queue_dst_ops);

    /**
     * Output frame queue methods
     */
    int (*set_frame_queue_dst_ops)(struct camera2_device *,
            camera2_metadata_queue_dst_ops *queue_dst_ops);

    int (*get_frame_queue_src_ops)(struct camera2_device *,
            camera2_metadata_queue_src_ops **queue_dst_ops);

    /**
     * Pass in notification methods
     */
    int (*set_notify_callback)(struct camera2_device *,
            camera2_notify_callback notify_cb);

    /**
     * Number of camera frames being processed by the device
     * at the moment (frames that have had their request dequeued,
     * but have not yet been enqueued onto output pipeline(s) )
     */
    int (*get_in_progress_count)(struct camera2_device *);

    /**
     * Flush all in-progress captures. This includes all dequeued requests
     * (regular or reprocessing) that have not yet placed any outputs into a
     * stream or the frame queue. Partially completed captures must be completed
     * normally. No new requests may be dequeued from the request or
     * reprocessing queues until the flush completes.
     */
    int (*flush_captures_in_progress)(struct camera2_device *);

    /**
     * Camera stream management
     */

    /**
     * Operations on the input reprocessing stream
     */
    int (*get_reprocess_stream_ops)(struct camera2_device *,
            camera2_stream_ops_t **stream_ops);

    /**
     * Get the number of streams that can be simultaneously allocated.
     * A request may include any allocated pipeline for its output, without
     * causing a substantial delay in frame production.
     */
    int (*get_stream_slot_count)(struct camera2_device *);

    /**
     * Allocate a new stream for use. Requires specifying which pipeline slot
     * to use. Specifies the buffer width, height, and format.
     * Error conditions:
     *  - Allocating an already-allocated slot without first releasing it
     *  - Requesting a width/height/format combination not listed as supported
     *  - Requesting a pipeline slot >= pipeline slot count.
     */
    int (*allocate_stream)(
        struct camera2_device *,
        uint32_t stream_slot,
        uint32_t width,
        uint32_t height,
        uint32_t format,
        camera2_stream_ops_t *camera2_stream_ops);

    /**
     * Release a stream. Returns an error if called when
     * get_in_progress_count is non-zero, or if the pipeline slot is not
     * allocated.
     */
    int (*release_stream)(
        struct camera2_device *,
        uint32_t stream_slot);

    /**
     * Get methods to query for vendor extension metadata tag infomation. May
     * set ops to NULL if no vendor extension tags are defined.
     */
    int (*get_metadata_vendor_tag_ops)(struct camera2_device*,
            vendor_tag_query_ops_t **ops);

    /**
     * Release the camera hardware.  Requests that are in flight will be
     * canceled. No further buffers will be pushed into any allocated pipelines
     * once this call returns.
     */
    void (*release)(struct camera2_device *);

    /**
     * Dump state of the camera hardware
     */
    int (*dump)(struct camera2_device *, int fd);

} camera2_device_ops_t;

typedef struct camera2_device {
    /**
     * common.version must equal CAMERA_DEVICE_API_VERSION_2_0 to identify
     * this device as implementing version 2.0 of the camera device HAL.
     */
    hw_device_t common;
    camera2_device_ops_t *ops;
    void *priv;
} camera2_device_t;

__END_DECLS

#endif /* #ifdef ANDROID_INCLUDE_CAMERA2_H */
