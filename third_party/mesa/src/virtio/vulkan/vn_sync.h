/*
 * Copyright 2026 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_SYNC_H
#define VN_SYNC_H

#include "vn_common.h"

#include "vn_feedback.h"

enum vn_sync_type {
   /* no payload */
   VN_SYNC_TYPE_INVALID,

   /* device object */
   VN_SYNC_TYPE_DEVICE_ONLY,

   /* payload is an imported sync file */
   VN_SYNC_TYPE_IMPORTED_SYNC_FD,
};

struct vn_sync_payload {
   enum vn_sync_type type;

   /* If type is VN_SYNC_TYPE_IMPORTED_SYNC_FD, fd is a sync file. */
   int fd;
};

/* For external fences and external semaphores submitted to be signaled. The
 * Vulkan spec guarantees those external syncs are on permanent payload.
 */
struct vn_sync_payload_external {
   /* ring_idx of the last queue submission */
   uint32_t ring_idx;
   /* valid when NO_ASYNC_QUEUE_SUBMIT perf option is not used */
   bool ring_seqno_valid;
   /* ring seqno of the last queue submission */
   uint32_t ring_seqno;
};

struct vn_fence {
   struct vn_object_base base;

   struct vn_sync_payload *payload;

   struct vn_sync_payload permanent;
   struct vn_sync_payload temporary;

   uint64_t signal_counter;
   struct vn_sync_feedback feedback;

   bool is_external;
   struct vn_sync_payload_external external_payload;
};
VK_DEFINE_NONDISP_HANDLE_CASTS(vn_fence,
                               base.vk,
                               VkFence,
                               VK_OBJECT_TYPE_FENCE)

struct vn_semaphore {
   struct vn_object_base base;

   VkSemaphoreType type;

   struct vn_sync_payload *payload;

   struct vn_sync_payload permanent;
   struct vn_sync_payload temporary;

   struct vn_sync_feedback feedback;

   bool is_external;
   struct vn_sync_payload_external external_payload;
};
VK_DEFINE_NONDISP_HANDLE_CASTS(vn_semaphore,
                               base.vk,
                               VkSemaphore,
                               VK_OBJECT_TYPE_SEMAPHORE)

struct vn_event {
   struct vn_object_base base;

   /* non-NULL if below are satisfied:
    * - event is created without VK_EVENT_CREATE_DEVICE_ONLY_BIT
    * - VN_PERF_NO_EVENT_FEEDBACK is disabled
    */
   struct vn_feedback_slot *feedback_slot;
};
VK_DEFINE_NONDISP_HANDLE_CASTS(vn_event,
                               base.vk,
                               VkEvent,
                               VK_OBJECT_TYPE_EVENT)

static inline bool
vn_semaphore_is_timeline(VkSemaphore sem_handle)
{
   struct vn_semaphore *sem = vn_semaphore_from_handle(sem_handle);
   return sem->type == VK_SEMAPHORE_TYPE_TIMELINE;
}

static inline bool
vn_semaphore_is_imported(VkSemaphore sem_handle)
{
   struct vn_semaphore *sem = vn_semaphore_from_handle(sem_handle);
   return sem->payload->type == VN_SYNC_TYPE_IMPORTED_SYNC_FD;
}

bool
vn_semaphore_wait_imported(VkDevice dev_handle, VkSemaphore sem_handle);

#endif /* VN_SYNC_H */
