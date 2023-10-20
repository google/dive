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

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "capture_service/log.h"
#include "capture_service/server.h"
#include "capture_service/trace_mgr.h"
#include "layer_common.h"
#include "loader_interfaces.h"
#include "xr_generated_dispatch_table.h"

#if defined(__GNUC__) && __GNUC__ >= 4
#    define LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#    define LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(_WIN32)
#    define LAYER_EXPORT __declspec(dllexport)
#else
#    define LAYER_EXPORT
#endif

namespace DiveLayer
{

namespace
{
const char kDiveXrLayerName[] = "XR_APILAYER_dive";
}

struct XrInstanceData
{
    XrInstance               instance;
    XrGeneratedDispatchTable dispatch_table;
    bool                     is_libwrap_loaded;
    std::thread              server_thread;

    XrInstanceData()
    {
        is_libwrap_loaded = IsLibwrapLoaded();
        LOGI("libwrap loaded: %d", is_libwrap_loaded);
        if (is_libwrap_loaded)
        {
            server_thread = std::thread(Dive::server_main);
        }
    }
    ~XrInstanceData()
    {
        if (is_libwrap_loaded && server_thread.joinable())
        {
            LOGI("Wait for server thread to join");
            server_thread.join();
        }
    }
};

struct XrSessionData
{
    XrSession                session;
    XrGeneratedDispatchTable dispatch_table;
};

static thread_local XrInstanceData *last_used_xr_instance_data = nullptr;
static thread_local XrSessionData  *last_used_xr_session_data = nullptr;

std::mutex                                                     g_xr_instance_mutex;
std::unordered_map<uintptr_t, std::unique_ptr<XrInstanceData>> g_xr_instance_data;

std::mutex                                                    g_xr_session_mutex;
std::unordered_map<uintptr_t, std::unique_ptr<XrSessionData>> g_xr_session_data;

XrInstanceData *GetXrInstanceLayerData(uintptr_t key)
{
    if (last_used_xr_instance_data && DataKey(last_used_xr_instance_data->instance) == key)
    {
        return last_used_xr_instance_data;
    }

    std::lock_guard<std::mutex> lock(g_xr_instance_mutex);
    auto                        iter = g_xr_instance_data.find(key);
    if (iter == g_xr_instance_data.end())
        return nullptr;

    last_used_xr_instance_data = iter->second.get();

    return last_used_xr_instance_data;
}

XrSessionData *GetXrSessionLayerData(uintptr_t key)
{
    if (last_used_xr_session_data && DataKey(last_used_xr_session_data->session) == key)
    {
        return last_used_xr_session_data;
    }

    std::lock_guard<std::mutex> lock(g_xr_session_mutex);
    auto                        iter = g_xr_session_data.find(key);
    if (iter == g_xr_session_data.end())
        return nullptr;
    last_used_xr_session_data = iter->second.get();
    return last_used_xr_session_data;
}

XRAPI_ATTR XrResult XRAPI_CALL ApiDiveLayerXrEndFrame(XrSession             session,
                                                      const XrFrameEndInfo *frameEndInfo)
{
    XrResult result = XrResult::XR_ERROR_HANDLE_INVALID;

    LOGD("ApiDiveLayerXrEndFrame key %lu, sess %p \n", DataKey(session), session);
    auto sess_data = GetXrSessionLayerData(DataKey(session));
    if (sess_data)
    {
        result = sess_data->dispatch_table.EndFrame(session, frameEndInfo);
    }
    else
    {
        LOGE("sess_data is null in ApiDiveLayerXrEndFrame\n");
    }
    Dive::GetTraceMgr().OnNewFrame();

    return result;
}

XRAPI_ATTR XrResult XRAPI_CALL
ApiDiveLayerXrCreateApiLayerInstance(const XrInstanceCreateInfo        *info,
                                     const struct XrApiLayerCreateInfo *apiLayerInfo,
                                     XrInstance                        *instance)
{

    LOGD("ApiDiveLayerXrCreateApiLayerInstance called.\n");
    PFN_xrGetInstanceProcAddr    next_get_instance_proc_addr = nullptr;
    PFN_xrCreateApiLayerInstance next_create_api_layer_instance = nullptr;
    XrApiLayerCreateInfo         new_api_layer_info = {};

    // Validate the API layer info and next API layer info structures before we
    // try to use them
    if (nullptr == apiLayerInfo ||
        XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO != apiLayerInfo->structType ||
        XR_API_LAYER_CREATE_INFO_STRUCT_VERSION > apiLayerInfo->structVersion ||
        sizeof(XrApiLayerCreateInfo) > apiLayerInfo->structSize ||
        nullptr == apiLayerInfo->nextInfo ||
        XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO != apiLayerInfo->nextInfo->structType ||
        XR_API_LAYER_NEXT_INFO_STRUCT_VERSION > apiLayerInfo->nextInfo->structVersion ||
        sizeof(XrApiLayerNextInfo) > apiLayerInfo->nextInfo->structSize ||
        0 != strcmp(kDiveXrLayerName, apiLayerInfo->nextInfo->layerName) ||
        nullptr == apiLayerInfo->nextInfo->nextGetInstanceProcAddr ||
        nullptr == apiLayerInfo->nextInfo->nextCreateApiLayerInstance)
    {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    // Copy the contents of the layer info struct, but then move the next info up
    // by one slot so that the next layer gets information.
    memcpy(&new_api_layer_info, apiLayerInfo, sizeof(XrApiLayerCreateInfo));
    new_api_layer_info.nextInfo = apiLayerInfo->nextInfo->next;

    // Get the function pointers we need
    next_get_instance_proc_addr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;
    next_create_api_layer_instance = apiLayerInfo->nextInfo->nextCreateApiLayerInstance;

    // Create the instance
    XrInstance returned_instance = *instance;
    XrResult result = next_create_api_layer_instance(info, &new_api_layer_info, &returned_instance);
    *instance = returned_instance;

    LOGD("Instance Created in %s\n", kDiveXrLayerName);
    auto id = std::make_unique<XrInstanceData>();
    id->instance = returned_instance;

    GeneratedXrPopulateDispatchTable(&id->dispatch_table,
                                     returned_instance,
                                     next_get_instance_proc_addr);

    {
        std::lock_guard<std::mutex> lock(g_xr_instance_mutex);
        auto                        key = DataKey(returned_instance);
        LOGD("key is %lu , instance is %p \n", key, returned_instance);
        g_xr_instance_data[key] = std::move(id);
    }

    return result;
}

XRAPI_ATTR XrResult XRAPI_CALL ApiDiveLayerXrDestroyInstance(XrInstance instance)
{

    LOGD("ApiDiveLayerXrDestroyInstance\n");
    XrResult result = XR_SUCCESS;

    auto sess_data = GetXrInstanceLayerData(DataKey(instance));
    if (sess_data)
    {
        result = sess_data->dispatch_table.DestroyInstance(instance);
    }

    {
        std::lock_guard<std::mutex> lock(g_xr_instance_mutex);
        g_xr_instance_data.erase(DataKey(instance));
        last_used_xr_instance_data = nullptr;
    }

    return result;
}

XRAPI_ATTR XrResult XRAPI_CALL ApiDiveLayerXrCreateSession(XrInstance                 instance,
                                                           const XrSessionCreateInfo *createInfo,
                                                           XrSession                 *session)
{
    XrResult result = XR_SUCCESS;
    auto     inst_data = GetXrInstanceLayerData(DataKey(instance));
    LOGD("ApiDiveLayerXrCreateSession DataKey(instance) is %lu , instance is %p \n",
         DataKey(instance),
         instance);

    if (inst_data)
    {
        result = inst_data->dispatch_table.CreateSession(instance, createInfo, session);
        LOGD("ApiDiveLayerXrCreateSession session created \n");
    }
    else
    {
        LOGE("inst_data is nullptr\n");
        result = XR_ERROR_INSTANCE_LOST;
        return result;
    }

    auto id = std::make_unique<XrSessionData>();
    id->session = *session;
    id->dispatch_table = inst_data->dispatch_table;
    {

        std::lock_guard<std::mutex> lock(g_xr_session_mutex);
        auto                        key = DataKey(*session);
        LOGD("ApiDiveLayerXrCreateSession key %lu, sess %p\n", key, *session);

        g_xr_session_data[key] = std::move(id);
    }

    return result;
}

XRAPI_ATTR XrResult XRAPI_CALL ApiDiveLayerXrDestroySession(XrSession session)
{
    LOGD("ApiDiveLayerXrDestroySession\n");
    XrResult result = XR_SUCCESS;

    auto sess_data = GetXrSessionLayerData(DataKey(session));
    if (sess_data)
    {
        result = sess_data->dispatch_table.DestroySession(session);
    }

    {
        std::lock_guard<std::mutex> lock(g_xr_session_mutex);
        g_xr_session_data.erase(DataKey(session));
    }

    return XrResult::XR_ERROR_HANDLE_INVALID;
}

XRAPI_ATTR XrResult XRAPI_CALL ApiDiveLayerXrGetInstanceProcAddr(XrInstance          instance,
                                                                 const char         *name,
                                                                 PFN_xrVoidFunction *function)
{

    XrResult    result = XR_ERROR_HANDLE_INVALID;
    std::string func_name = name;
    LOGD("xrGetInstanceProcAddr:  %s\n", func_name.c_str());

    if (func_name == "xrCreateSession")
    {
        *function = reinterpret_cast<PFN_xrVoidFunction>(ApiDiveLayerXrCreateSession);
        return XR_SUCCESS;
    }
    else if (func_name == "xrDestroyInstance")
    {
        *function = reinterpret_cast<PFN_xrVoidFunction>(ApiDiveLayerXrDestroyInstance);
        return XR_SUCCESS;
    }

    else if (func_name == "xrDestroySession")
    {
        *function = reinterpret_cast<PFN_xrVoidFunction>(ApiDiveLayerXrDestroySession);
        return XR_SUCCESS;
    }
    else if (func_name == "xrEndFrame")
    {
        *function = reinterpret_cast<PFN_xrVoidFunction>(ApiDiveLayerXrEndFrame);
        return XR_SUCCESS;
    }

    auto instance_data = GetXrInstanceLayerData(DataKey(instance));
    if (instance_data == nullptr)
    {
        *function = nullptr;
        return XR_ERROR_HANDLE_INVALID;
    }

    return instance_data->dispatch_table.GetInstanceProcAddr(instance, name, function);
}

extern "C"
{
    LAYER_EXPORT XRAPI_ATTR XrResult XRAPI_CALL
    xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo *loaderInfo,
                                       const char * /*apiLayerName*/,
                                       XrNegotiateApiLayerRequest *apiLayerRequest)
    {

        LOGD("%s : xrNegotiateLoaderApiLayerInterface\n", kDiveXrLayerName);
        if (nullptr == loaderInfo || nullptr == apiLayerRequest ||
            loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
            loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
            loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo) ||
            apiLayerRequest->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
            apiLayerRequest->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION ||
            apiLayerRequest->structSize != sizeof(XrNegotiateApiLayerRequest) ||
            loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxApiVersion < XR_CURRENT_API_VERSION ||
            loaderInfo->minApiVersion > XR_CURRENT_API_VERSION)
        {
            return XR_ERROR_INITIALIZATION_FAILED;
        }

        apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
        apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
        apiLayerRequest->getInstanceProcAddr = ApiDiveLayerXrGetInstanceProcAddr;
        apiLayerRequest->createApiLayerInstance = ApiDiveLayerXrCreateApiLayerInstance;

        return XR_SUCCESS;
    }
}
}  // namespace DiveLayer