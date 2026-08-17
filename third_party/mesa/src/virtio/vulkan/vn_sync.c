/*
 * Copyright 2026 Google LLC
 * SPDX-License-Identifier: MIT
 */

#include "vn_sync.h"

#include "venus-protocol/vn_protocol_driver_event.h"
#include "venus-protocol/vn_protocol_driver_fence.h"
#include "venus-protocol/vn_protocol_driver_semaphore.h"
#include "venus-protocol/vn_protocol_driver_transport.h"

#include "vn_device.h"
#include "vn_physical_device.h"
#include "vn_queue.h"
#include "vn_renderer.h"
#include "vn_wsi.h"

static void
vn_sync_payload_release(UNUSED struct vn_device *dev,
                        struct vn_sync_payload *payload)
{
   if (payload->type == VN_SYNC_TYPE_IMPORTED_SYNC_FD && payload->fd >= 0)
      close(payload->fd);

   payload->type = VN_SYNC_TYPE_INVALID;
}

/* fence commands */

static VkResult
vn_fence_init_payloads(struct vn_device *dev,
                       struct vn_fence *fence,
                       bool signaled,
                       const VkAllocationCallbacks *alloc)
{
   fence->permanent.type = VN_SYNC_TYPE_DEVICE_ONLY;
   fence->temporary.type = VN_SYNC_TYPE_INVALID;
   fence->payload = &fence->permanent;

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_CreateFence(VkDevice device,
               const VkFenceCreateInfo *pCreateInfo,
               const VkAllocationCallbacks *pAllocator,
               VkFence *pFence)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
      pAllocator ? pAllocator : &dev->base.vk.alloc;
   const bool signaled = pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT;
   VkResult result;

   struct vn_fence *fence = vk_zalloc(alloc, sizeof(*fence), VN_DEFAULT_ALIGN,
                                      VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!fence)
      return vn_error(dev->instance, VK_ERROR_OUT_OF_HOST_MEMORY);

   vn_object_base_init(&fence->base, VK_OBJECT_TYPE_FENCE, &dev->base);

   const struct VkExportFenceCreateInfo *export_info =
      vk_find_struct_const(pCreateInfo->pNext, EXPORT_FENCE_CREATE_INFO);
   fence->is_external = export_info && export_info->handleTypes;

   result = vn_fence_init_payloads(dev, fence, signaled, alloc);
   if (result != VK_SUCCESS)
      goto out_object_base_fini;

   if (!fence->is_external && !VN_PERF(NO_FENCE_FEEDBACK)) {
      fence->signal_counter = signaled ? 0 : 1;
      result = vn_sync_feedback_init(dev, &fence->feedback, 0);
      if (result != VK_SUCCESS)
         goto out_payloads_fini;
   }

   *pFence = vn_fence_to_handle(fence);
   vn_async_vkCreateFence(dev->primary_ring, device, pCreateInfo, NULL,
                          pFence);

   return VK_SUCCESS;

out_payloads_fini:
   vn_sync_payload_release(dev, &fence->permanent);
   vn_sync_payload_release(dev, &fence->temporary);

out_object_base_fini:
   vn_object_base_fini(&fence->base);
   vk_free(alloc, fence);
   return vn_error(dev->instance, result);
}

VKAPI_ATTR void VKAPI_CALL
vn_DestroyFence(VkDevice device,
                VkFence _fence,
                const VkAllocationCallbacks *pAllocator)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_fence *fence = vn_fence_from_handle(_fence);
   const VkAllocationCallbacks *alloc =
      pAllocator ? pAllocator : &dev->base.vk.alloc;

   if (!fence)
      return;

   vn_async_vkDestroyFence(dev->primary_ring, device, _fence, NULL);

   vn_sync_feedback_fini(dev, &fence->feedback);

   vn_sync_payload_release(dev, &fence->permanent);
   vn_sync_payload_release(dev, &fence->temporary);

   vn_object_base_fini(&fence->base);
   vk_free(alloc, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_ResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);

   vn_async_vkResetFences(dev->primary_ring, device, fenceCount, pFences);

   for (uint32_t i = 0; i < fenceCount; i++) {
      struct vn_fence *fence = vn_fence_from_handle(pFences[i]);
      struct vn_sync_payload *perm = &fence->permanent;

      vn_sync_payload_release(dev, &fence->temporary);

      assert(perm->type == VN_SYNC_TYPE_DEVICE_ONLY);
      fence->payload = perm;

      if (vn_sync_feedback_enabled(&fence->feedback)) {
         vn_sync_feedback_try_resume(&fence->feedback,
                                     fence->signal_counter++);
      }
   }

   return VK_SUCCESS;
}

static VkResult
vn_get_fence_status(VkDevice dev_handle,
                    VkFence fence_handle,
                    struct vn_relax_state *relax_state)
{
   struct vn_device *dev = vn_device_from_handle(dev_handle);
   struct vn_fence *fence = vn_fence_from_handle(fence_handle);
   struct vn_sync_payload *payload = fence->payload;

   VkResult result;
   switch (payload->type) {
   case VN_SYNC_TYPE_DEVICE_ONLY:
      if (vn_sync_feedback_pollable(&fence->feedback)) {
         if (relax_state && vn_relax_warn(relax_state)) {
            /* Upon vn_relax warn order, emit a synchronous vkGetFenceStatus
             * to catch renderer device lost.
             */
            result = vn_call_vkGetFenceStatus(dev->primary_ring, dev_handle,
                                              fence_handle);
            if (result == VK_ERROR_DEVICE_LOST) {
               vn_log(dev->instance, "aborting on ffb device lost");
               abort();
            }
            if (result != VK_SUCCESS)
               return result;
         }

         uint64_t counter = 0;
         if (vn_sync_feedback_query(dev, &fence->feedback, &counter)) {
            /* When fence feedback slot gets signaled, the real fence
             * signal operation follows after but the signaling isr can be
             * deferred or preempted. To avoid racing, we let the
             * renderer wait for the fence. This also helps resolve
             * synchronization validation errors, because the layer no
             * longer sees any fence status checks and falsely believes the
             * caller does not sync.
             */
            vn_async_vkWaitForFences(dev->primary_ring, dev_handle, 1,
                                     &fence_handle, VK_TRUE, UINT64_MAX);
         }

         const bool signaled = counter == fence->signal_counter;
         result = signaled ? VK_SUCCESS : VK_NOT_READY;
      } else {
         result = vn_call_vkGetFenceStatus(dev->primary_ring, dev_handle,
                                           fence_handle);
      }
      break;
   case VN_SYNC_TYPE_IMPORTED_SYNC_FD:
      if (payload->fd < 0 || sync_wait(payload->fd, 0) == 0)
         result = VK_SUCCESS;
      else
         result = errno == ETIME ? VK_NOT_READY : VK_ERROR_DEVICE_LOST;
      break;
   default:
      UNREACHABLE("unexpected fence payload type");
      break;
   }

   return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_GetFenceStatus(VkDevice device, VkFence fence)
{
   struct vn_device *dev = vn_device_from_handle(device);
   VkResult result = vn_get_fence_status(device, fence, NULL);
   return vn_result(dev->instance, result);
}

static VkResult
vn_find_first_signaled_fence(VkDevice device,
                             const VkFence *fences,
                             uint32_t count,
                             struct vn_relax_state *relax_state)
{
   for (uint32_t i = 0; i < count; i++) {
      VkResult result = vn_get_fence_status(device, fences[i], relax_state);
      if (result == VK_SUCCESS || result < 0)
         return result;
   }
   return VK_NOT_READY;
}

static VkResult
vn_remove_signaled_fences(VkDevice device,
                          VkFence *fences,
                          uint32_t *count,
                          struct vn_relax_state *relax_state)
{
   uint32_t cur = 0;
   for (uint32_t i = 0; i < *count; i++) {
      VkResult result = vn_get_fence_status(device, fences[i], relax_state);
      if (result != VK_SUCCESS) {
         if (result < 0)
            return result;
         fences[cur++] = fences[i];
      }
   }

   *count = cur;
   return cur ? VK_NOT_READY : VK_SUCCESS;
}

static VkResult
vn_update_sync_result(struct vn_device *dev,
                      VkResult result,
                      int64_t abs_timeout,
                      struct vn_relax_state *relax_state)
{
   switch (result) {
   case VK_NOT_READY:
      if (abs_timeout != OS_TIMEOUT_INFINITE &&
          os_time_get_nano() >= abs_timeout)
         result = VK_TIMEOUT;
      else
         vn_relax(relax_state);
      break;
   default:
      assert(result == VK_SUCCESS || result < 0);
      break;
   }

   return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_WaitForFences(VkDevice device,
                 uint32_t fenceCount,
                 const VkFence *pFences,
                 VkBool32 waitAll,
                 uint64_t timeout)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);

   const int64_t abs_timeout = os_time_get_absolute_timeout(timeout);
   VkResult result = VK_NOT_READY;
   if (fenceCount > 1 && waitAll) {
      STACK_ARRAY(VkFence, fences, fenceCount);
      typed_memcpy(fences, pFences, fenceCount);

      struct vn_relax_state relax_state =
         vn_relax_init(dev->instance, VN_RELAX_REASON_FENCE);
      while (result == VK_NOT_READY) {
         result = vn_remove_signaled_fences(device, fences, &fenceCount,
                                            &relax_state);
         result =
            vn_update_sync_result(dev, result, abs_timeout, &relax_state);
      }
      vn_relax_fini(&relax_state);

      STACK_ARRAY_FINISH(fences);
   } else {
      struct vn_relax_state relax_state =
         vn_relax_init(dev->instance, VN_RELAX_REASON_FENCE);
      while (result == VK_NOT_READY) {
         result = vn_find_first_signaled_fence(device, pFences, fenceCount,
                                               &relax_state);
         result =
            vn_update_sync_result(dev, result, abs_timeout, &relax_state);
      }
      vn_relax_fini(&relax_state);
   }

   return vn_result(dev->instance, result);
}

static VkResult
vn_create_sync_file(struct vn_device *dev,
                    struct vn_sync_payload_external *external_payload,
                    int *out_fd)
{
   struct vn_renderer_sync *sync;
   VkResult result = vn_renderer_sync_create(dev->renderer, 0,
                                             VN_RENDERER_SYNC_BINARY, &sync);
   if (result != VK_SUCCESS)
      return vn_error(dev->instance, result);

   struct vn_renderer_submit_batch batch = {
      .syncs = &sync,
      .sync_values = &(const uint64_t){ 1 },
      .sync_count = 1,
      .ring_idx = external_payload->ring_idx,
   };

   uint32_t local_data[8];
   struct vn_cs_encoder local_enc =
      VN_CS_ENCODER_INITIALIZER_LOCAL(local_data, sizeof(local_data));
   if (external_payload->ring_seqno_valid) {
      const uint64_t ring_id = vn_ring_get_id(dev->primary_ring);
      vn_encode_vkWaitRingSeqnoMESA(&local_enc, 0, ring_id,
                                    external_payload->ring_seqno);
      batch.cs_data = local_data;
      batch.cs_size = vn_cs_encoder_get_len(&local_enc);
   }

   result = vn_renderer_submit(dev->renderer, &batch);
   if (result != VK_SUCCESS) {
      vn_renderer_sync_destroy(dev->renderer, sync);
      return vn_error(dev->instance, result);
   }

   *out_fd = vn_renderer_sync_export_syncobj(dev->renderer, sync, true);
   vn_renderer_sync_destroy(dev->renderer, sync);

   return *out_fd >= 0 ? VK_SUCCESS : VK_ERROR_TOO_MANY_OBJECTS;
}

static inline bool
vn_sync_valid_fd(int fd)
{
   /* the special value -1 for fd is treated like a valid sync file descriptor
    * referring to an object that has already signaled
    */
   return (fd >= 0 && sync_valid_fd(fd)) || fd == -1;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_ImportFenceFdKHR(VkDevice device,
                    const VkImportFenceFdInfoKHR *pImportFenceFdInfo)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_fence *fence = vn_fence_from_handle(pImportFenceFdInfo->fence);
   ASSERTED const bool sync_file = pImportFenceFdInfo->handleType ==
                                   VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;
   const int fd = pImportFenceFdInfo->fd;

   assert(sync_file);

   if (!vn_sync_valid_fd(fd))
      return vn_error(dev->instance, VK_ERROR_INVALID_EXTERNAL_HANDLE);

   struct vn_sync_payload *temp = &fence->temporary;
   vn_sync_payload_release(dev, temp);
   temp->type = VN_SYNC_TYPE_IMPORTED_SYNC_FD;
   temp->fd = fd;
   fence->payload = temp;

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_GetFenceFdKHR(VkDevice device,
                 const VkFenceGetFdInfoKHR *pGetFdInfo,
                 int *pFd)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_fence *fence = vn_fence_from_handle(pGetFdInfo->fence);
   const bool sync_file =
      pGetFdInfo->handleType == VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;
   struct vn_sync_payload *payload = fence->payload;
   VkResult result;

   assert(sync_file);
   assert(dev->physical_device->renderer_sync_fd.fence_exportable);

   int fd = -1;
   if (payload->type == VN_SYNC_TYPE_DEVICE_ONLY) {
      result = vn_create_sync_file(dev, &fence->external_payload, &fd);
      if (result != VK_SUCCESS)
         return vn_error(dev->instance, result);

      vn_async_vkResetFenceResourceMESA(dev->primary_ring, device,
                                        pGetFdInfo->fence);

      vn_sync_payload_release(dev, &fence->temporary);
      fence->payload = &fence->permanent;
   } else {
      assert(payload->type == VN_SYNC_TYPE_IMPORTED_SYNC_FD);

      /* transfer ownership of imported sync fd to save a dup */
      fd = payload->fd;
      payload->fd = -1;

      vn_sync_payload_release(dev, payload);
      fence->payload = &fence->permanent;
   }

   *pFd = fd;
   return VK_SUCCESS;
}

/* semaphore commands */

bool
vn_semaphore_wait_imported(VkDevice dev_handle, VkSemaphore sem_handle)
{
   struct vn_device *dev = vn_device_from_handle(dev_handle);
   struct vn_semaphore *sem = vn_semaphore_from_handle(sem_handle);
   const struct vn_sync_payload *payload = sem->payload;

   assert(payload->type == VN_SYNC_TYPE_IMPORTED_SYNC_FD &&
          payload == &sem->temporary);

   if (payload->fd >= 0) {
      if (sync_wait(payload->fd, -1))
         return false;
   }

   vn_sync_payload_release(dev, &sem->temporary);
   sem->payload = &sem->permanent;

   return true;
}

static VkResult
vn_semaphore_init_payloads(struct vn_device *dev,
                           struct vn_semaphore *sem,
                           uint64_t initial_val,
                           const VkAllocationCallbacks *alloc)
{
   sem->permanent.type = VN_SYNC_TYPE_DEVICE_ONLY;
   sem->temporary.type = VN_SYNC_TYPE_INVALID;
   sem->payload = &sem->permanent;

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_CreateSemaphore(VkDevice device,
                   const VkSemaphoreCreateInfo *pCreateInfo,
                   const VkAllocationCallbacks *pAllocator,
                   VkSemaphore *pSemaphore)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
      pAllocator ? pAllocator : &dev->base.vk.alloc;

   struct vn_semaphore *sem = vk_zalloc(alloc, sizeof(*sem), VN_DEFAULT_ALIGN,
                                        VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!sem)
      return vn_error(dev->instance, VK_ERROR_OUT_OF_HOST_MEMORY);

   vn_object_base_init(&sem->base, VK_OBJECT_TYPE_SEMAPHORE, &dev->base);

   const VkSemaphoreTypeCreateInfo *type_info =
      vk_find_struct_const(pCreateInfo->pNext, SEMAPHORE_TYPE_CREATE_INFO);
   uint64_t initial_val = 0;
   if (type_info && type_info->semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE) {
      sem->type = VK_SEMAPHORE_TYPE_TIMELINE;
      initial_val = type_info->initialValue;
   } else {
      sem->type = VK_SEMAPHORE_TYPE_BINARY;
   }

   const struct VkExportSemaphoreCreateInfo *export_info =
      vk_find_struct_const(pCreateInfo->pNext, EXPORT_SEMAPHORE_CREATE_INFO);
   sem->is_external = export_info && export_info->handleTypes;

   VkResult result = vn_semaphore_init_payloads(dev, sem, initial_val, alloc);
   if (result != VK_SUCCESS)
      goto out_object_base_fini;

   if (sem->type == VK_SEMAPHORE_TYPE_TIMELINE &&
       !VN_PERF(NO_SEMAPHORE_FEEDBACK)) {
      assert(!sem->is_external);

      result = vn_sync_feedback_init(dev, &sem->feedback, initial_val);
      if (result != VK_SUCCESS)
         goto out_payloads_fini;
   }

   VkSemaphore sem_handle = vn_semaphore_to_handle(sem);
   vn_async_vkCreateSemaphore(dev->primary_ring, device, pCreateInfo, NULL,
                              &sem_handle);

   *pSemaphore = sem_handle;

   return VK_SUCCESS;

out_payloads_fini:
   vn_sync_payload_release(dev, &sem->permanent);
   vn_sync_payload_release(dev, &sem->temporary);

out_object_base_fini:
   vn_object_base_fini(&sem->base);
   vk_free(alloc, sem);
   return vn_error(dev->instance, result);
}

VKAPI_ATTR void VKAPI_CALL
vn_DestroySemaphore(VkDevice device,
                    VkSemaphore semaphore,
                    const VkAllocationCallbacks *pAllocator)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem = vn_semaphore_from_handle(semaphore);
   const VkAllocationCallbacks *alloc =
      pAllocator ? pAllocator : &dev->base.vk.alloc;

   if (!sem)
      return;

   vn_async_vkDestroySemaphore(dev->primary_ring, device, semaphore, NULL);

   if (sem->type == VK_SEMAPHORE_TYPE_TIMELINE)
      vn_sync_feedback_fini(dev, &sem->feedback);

   vn_sync_payload_release(dev, &sem->permanent);
   vn_sync_payload_release(dev, &sem->temporary);

   vn_object_base_fini(&sem->base);
   vk_free(alloc, sem);
}

static VkResult
vn_get_semaphore_counter_value(VkDevice dev_handle,
                               VkSemaphore sem_handle,
                               struct vn_relax_state *relax_state,
                               uint64_t *out_value)
{
   struct vn_device *dev = vn_device_from_handle(dev_handle);
   struct vn_semaphore *sem = vn_semaphore_from_handle(sem_handle);
   ASSERTED struct vn_sync_payload *payload = sem->payload;

   assert(payload->type == VN_SYNC_TYPE_DEVICE_ONLY);

   if (vn_sync_feedback_pollable(&sem->feedback)) {
      if (relax_state && vn_relax_warn(relax_state)) {
         /* Emit a synchronous vkGetSemaphoreCounterValue to catch renderer
          * device lost without tangling with sfb internals.
          */
         VkResult result = vn_call_vkGetSemaphoreCounterValue(
            dev->primary_ring, dev_handle, sem_handle, out_value);
         if (result == VK_ERROR_DEVICE_LOST) {
            vn_log(dev->instance, "aborting on sfb device lost");
            abort();
         }
         if (result != VK_SUCCESS)
            return result;
      }

      if (vn_sync_feedback_query(dev, &sem->feedback, out_value)) {
         /* When the timeline semaphore feedback slot gets signaled, the real
          * semaphore signal operation follows after but the signaling isr can
          * be deferred or preempted. To avoid racing, we let the renderer
          * wait for the semaphore by sending an asynchronous wait call for
          * the feedback value.
          * We also cache the counter value to only send the async call once
          * per counter value to prevent spamming redundant async wait calls.
          * The cached counter value requires a lock to ensure multiple
          * threads querying for the same value are guaranteed to encode after
          * the async wait call.
          *
          * This also helps resolve synchronization validation errors, because
          * the layer no longer sees any semaphore status checks and falsely
          * believes the caller does not sync.
          */
         VkSemaphoreWaitInfo wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = NULL,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &sem_handle,
            .pValues = out_value,
         };

         vn_async_vkWaitSemaphores(dev->primary_ring, dev_handle, &wait_info,
                                   UINT64_MAX);
      }
   } else {
      VkResult result = vn_call_vkGetSemaphoreCounterValue(
         dev->primary_ring, dev_handle, sem_handle, out_value);
      if (result != VK_SUCCESS)
         return result;

      if (vn_sync_feedback_enabled(&sem->feedback))
         vn_sync_feedback_try_resume(&sem->feedback, *out_value);
   }

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_GetSemaphoreCounterValue(VkDevice device,
                            VkSemaphore semaphore,
                            uint64_t *pValue)
{
   struct vn_device *dev = vn_device_from_handle(device);
   VkResult result =
      vn_get_semaphore_counter_value(device, semaphore, NULL, pValue);
   return vn_result(dev->instance, result);
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_SignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem =
      vn_semaphore_from_handle(pSignalInfo->semaphore);

   vn_async_vkSignalSemaphore(dev->primary_ring, device, pSignalInfo);

   if (vn_sync_feedback_enabled(&sem->feedback))
      vn_sync_feedback_write(&sem->feedback, pSignalInfo->value);

   return VK_SUCCESS;
}

static VkResult
vn_find_first_signaled_semaphore(VkDevice device,
                                 const VkSemaphore *semaphores,
                                 const uint64_t *values,
                                 uint32_t count,
                                 struct vn_relax_state *relax_state)
{
   for (uint32_t i = 0; i < count; i++) {
      uint64_t val = 0;
      VkResult result = vn_get_semaphore_counter_value(device, semaphores[i],
                                                       relax_state, &val);
      if (result != VK_SUCCESS || val >= values[i])
         return result;
   }
   return VK_NOT_READY;
}

static VkResult
vn_remove_signaled_semaphores(VkDevice device,
                              VkSemaphore *semaphores,
                              uint64_t *values,
                              uint32_t *count,
                              struct vn_relax_state *relax_state)
{
   uint32_t cur = 0;
   for (uint32_t i = 0; i < *count; i++) {
      uint64_t val = 0;
      VkResult result = vn_get_semaphore_counter_value(device, semaphores[i],
                                                       relax_state, &val);
      if (result != VK_SUCCESS)
         return result;
      if (val < values[i])
         semaphores[cur++] = semaphores[i];
   }

   *count = cur;
   return cur ? VK_NOT_READY : VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_WaitSemaphores(VkDevice device,
                  const VkSemaphoreWaitInfo *pWaitInfo,
                  uint64_t timeout)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);

   const int64_t abs_timeout = os_time_get_absolute_timeout(timeout);
   VkResult result = VK_NOT_READY;
   if (pWaitInfo->semaphoreCount > 1 &&
       !(pWaitInfo->flags & VK_SEMAPHORE_WAIT_ANY_BIT)) {
      uint32_t semaphore_count = pWaitInfo->semaphoreCount;
      STACK_ARRAY(VkSemaphore, semaphores, semaphore_count);
      STACK_ARRAY(uint64_t, values, semaphore_count);
      typed_memcpy(semaphores, pWaitInfo->pSemaphores, semaphore_count);
      typed_memcpy(values, pWaitInfo->pValues, semaphore_count);

      struct vn_relax_state relax_state =
         vn_relax_init(dev->instance, VN_RELAX_REASON_SEMAPHORE);
      while (result == VK_NOT_READY) {
         result = vn_remove_signaled_semaphores(
            device, semaphores, values, &semaphore_count, &relax_state);
         result =
            vn_update_sync_result(dev, result, abs_timeout, &relax_state);
      }
      vn_relax_fini(&relax_state);

      STACK_ARRAY_FINISH(semaphores);
      STACK_ARRAY_FINISH(values);
   } else {
      struct vn_relax_state relax_state =
         vn_relax_init(dev->instance, VN_RELAX_REASON_SEMAPHORE);
      while (result == VK_NOT_READY) {
         result = vn_find_first_signaled_semaphore(
            device, pWaitInfo->pSemaphores, pWaitInfo->pValues,
            pWaitInfo->semaphoreCount, &relax_state);
         result =
            vn_update_sync_result(dev, result, abs_timeout, &relax_state);
      }
      vn_relax_fini(&relax_state);
   }

   return vn_result(dev->instance, result);
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_ImportSemaphoreFdKHR(
   VkDevice device, const VkImportSemaphoreFdInfoKHR *pImportSemaphoreFdInfo)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem =
      vn_semaphore_from_handle(pImportSemaphoreFdInfo->semaphore);
   ASSERTED const bool sync_file =
      pImportSemaphoreFdInfo->handleType ==
      VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
   const int fd = pImportSemaphoreFdInfo->fd;

   assert(sync_file);

   if (!vn_sync_valid_fd(fd))
      return vn_error(dev->instance, VK_ERROR_INVALID_EXTERNAL_HANDLE);

   struct vn_sync_payload *temp = &sem->temporary;
   vn_sync_payload_release(dev, temp);
   temp->type = VN_SYNC_TYPE_IMPORTED_SYNC_FD;
   temp->fd = fd;
   sem->payload = temp;

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_GetSemaphoreFdKHR(VkDevice device,
                     const VkSemaphoreGetFdInfoKHR *pGetFdInfo,
                     int *pFd)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem = vn_semaphore_from_handle(pGetFdInfo->semaphore);
   const bool sync_file =
      pGetFdInfo->handleType == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
   struct vn_sync_payload *payload = sem->payload;

   assert(sync_file);
   assert(dev->physical_device->renderer_sync_fd.semaphore_exportable);

   int fd = -1;
   if (payload->type == VN_SYNC_TYPE_DEVICE_ONLY) {
      VkResult result = vn_create_sync_file(dev, &sem->external_payload, &fd);
      if (result != VK_SUCCESS)
         return vn_error(dev->instance, result);

      vn_wsi_sync_wait(dev, fd);
   } else {
      assert(payload->type == VN_SYNC_TYPE_IMPORTED_SYNC_FD);

      /* transfer ownership of imported sync fd to save a dup */
      fd = payload->fd;
      payload->fd = -1;
   }

   /* no need to wait for renderer side semaphore for imported sync */
   if (payload->type != VN_SYNC_TYPE_IMPORTED_SYNC_FD) {
      vn_async_vkWaitSemaphoreResourceMESA(dev->primary_ring, device,
                                           pGetFdInfo->semaphore);
   }

   vn_sync_payload_release(dev, &sem->temporary);
   sem->payload = &sem->permanent;

   *pFd = fd;
   return VK_SUCCESS;
}

/* event commands */

static VkResult
vn_event_feedback_init(struct vn_device *dev, struct vn_event *ev)
{
   struct vn_feedback_slot *slot;

   if (VN_PERF(NO_EVENT_FEEDBACK))
      return VK_SUCCESS;

   slot = vn_feedback_pool_alloc(&dev->feedback_pool, VN_FEEDBACK_TYPE_EVENT);
   if (!slot)
      return VK_ERROR_OUT_OF_HOST_MEMORY;

   /* newly created event object is in the unsignaled state */
   vn_feedback_set_status(slot, VK_EVENT_RESET);

   ev->feedback_slot = slot;

   return VK_SUCCESS;
}

static inline void
vn_event_feedback_fini(struct vn_device *dev, struct vn_event *ev)
{
   if (ev->feedback_slot)
      vn_feedback_pool_free(&dev->feedback_pool, ev->feedback_slot);
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_CreateEvent(VkDevice device,
               const VkEventCreateInfo *pCreateInfo,
               const VkAllocationCallbacks *pAllocator,
               VkEvent *pEvent)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
      pAllocator ? pAllocator : &dev->base.vk.alloc;

   struct vn_event *ev = vk_zalloc(alloc, sizeof(*ev), VN_DEFAULT_ALIGN,
                                   VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!ev)
      return vn_error(dev->instance, VK_ERROR_OUT_OF_HOST_MEMORY);

   vn_object_base_init(&ev->base, VK_OBJECT_TYPE_EVENT, &dev->base);

   /* feedback is only needed to speed up host operations */
   if (!(pCreateInfo->flags & VK_EVENT_CREATE_DEVICE_ONLY_BIT)) {
      VkResult result = vn_event_feedback_init(dev, ev);
      if (result != VK_SUCCESS)
         return vn_error(dev->instance, result);
   }

   VkEvent ev_handle = vn_event_to_handle(ev);
   vn_async_vkCreateEvent(dev->primary_ring, device, pCreateInfo, NULL,
                          &ev_handle);

   *pEvent = ev_handle;

   return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vn_DestroyEvent(VkDevice device,
                VkEvent event,
                const VkAllocationCallbacks *pAllocator)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_event *ev = vn_event_from_handle(event);
   const VkAllocationCallbacks *alloc =
      pAllocator ? pAllocator : &dev->base.vk.alloc;

   if (!ev)
      return;

   vn_async_vkDestroyEvent(dev->primary_ring, device, event, NULL);

   vn_event_feedback_fini(dev, ev);

   vn_object_base_fini(&ev->base);
   vk_free(alloc, ev);
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_GetEventStatus(VkDevice device, VkEvent event)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_event *ev = vn_event_from_handle(event);
   VkResult result;

   if (ev->feedback_slot)
      result = vn_feedback_get_status(ev->feedback_slot);
   else
      result = vn_call_vkGetEventStatus(dev->primary_ring, device, event);

   return vn_result(dev->instance, result);
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_SetEvent(VkDevice device, VkEvent event)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_event *ev = vn_event_from_handle(event);

   if (ev->feedback_slot) {
      vn_feedback_set_status(ev->feedback_slot, VK_EVENT_SET);
      vn_async_vkSetEvent(dev->primary_ring, device, event);
   } else {
      VkResult result = vn_call_vkSetEvent(dev->primary_ring, device, event);
      if (result != VK_SUCCESS)
         return vn_error(dev->instance, result);
   }

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vn_ResetEvent(VkDevice device, VkEvent event)
{
   VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_event *ev = vn_event_from_handle(event);

   if (ev->feedback_slot) {
      vn_feedback_reset_status(ev->feedback_slot);
      vn_async_vkResetEvent(dev->primary_ring, device, event);
   } else {
      VkResult result =
         vn_call_vkResetEvent(dev->primary_ring, device, event);
      if (result != VK_SUCCESS)
         return vn_error(dev->instance, result);
   }

   return VK_SUCCESS;
}
