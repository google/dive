/*
Copyright 2023 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "vk_layer_impl.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#if defined(__ANDROID__)
#    include <dlfcn.h>
#endif

#include <vulkan/vulkan_core.h>

#include "capture_service/log.h"
#include "capture_service/trace_mgr.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
namespace DiveLayer
{

VkResult QueuePresentKHR(PFN_vkQueuePresentKHR   pfn,
                         VkQueue                 queue,
                         const VkPresentInfoKHR *pPresentInfo)
{
    VkResult ret = pfn(queue, pPresentInfo);
    Dive::GetTraceMgr().OnNewFrame();
    return ret;
}

}  // namespace DiveLayer