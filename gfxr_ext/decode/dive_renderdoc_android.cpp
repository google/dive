/*
Copyright 2025 Google Inc.

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

#include "dive_renderdoc.h"

#include <dlfcn.h>

#include "util/defines.h"
#include "util/logging.h"
#include "third_party/renderdoc/renderdoc_app.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

namespace
{

RENDERDOC_API_1_0_0* LoadRenderDocApi()
{
    // Could use util::platform::OpenLibrary() except this only needs to run on Android and the
    // flags we want are different.
    void* renderdoc_so = dlopen("libVkLayer_GLES_RenderDoc.so", RTLD_NOW | RTLD_NOLOAD);
    if (renderdoc_so == nullptr)
    {
        GFXRECON_LOG_INFO("Couldn't dlopen libVkLayer_GLES_RenderDoc.so. If you didn't request a "
                          "RenderDoc capture, this is expected. Error: %s",
                          dlerror());
        return nullptr;
    }

    (void)dlerror();  // Clear previous error, suggested by docs
    pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(
    dlsym(renderdoc_so, "RENDERDOC_GetAPI"));
    if (RENDERDOC_GetAPI == nullptr)
    {
        GFXRECON_LOG_INFO("Failed to dlsym RENDERDOC_GetAPI: %s", dlerror());
        return nullptr;
    }

    // API v1.0.0 was chosen since we don't need newer API features and it enables maximum
    // compatiblity (release v0.26 and later).
    RENDERDOC_API_1_0_0* renderdoc_api = nullptr;
    if (RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, reinterpret_cast<void**>(&renderdoc_api)) !=
        1)
    {
        GFXRECON_LOG_INFO("RENDERDOC_GetAPI failed");
        return nullptr;
    }

    int major = 0;
    int minor = 0;
    int patch = 0;
    renderdoc_api->GetAPIVersion(&major, &minor, &patch);
    GFXRECON_LOG_INFO("Loaded RenderDoc In-Application API v%d.%d.%d", major, minor, patch);
    return renderdoc_api;
}

}  // namespace

const RENDERDOC_API_1_0_0* GetRenderDocApi()
{
    // Singleton; this points to memory owned by RenderDoc capture layer and becomes invalid when
    // the library implementing the layer is unloaded. It is assumed that, for RenderDoc capture,
    // the layer is loaded before the application starts and is unloaded when the application quits.
    // If that's not the case, we could consider checking whether the singleton is valid on each
    // call instead of only trying to create it once.
    static const RENDERDOC_API_1_0_0* const renderdoc_api = LoadRenderDocApi();
    return renderdoc_api;
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
