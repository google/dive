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

// Fix warning 4003. util/platform.h (included by util/logging.h) includes windows.h (which defines
// max as a macro) but calls std::numeric_limits::max() (which the proprocessor substitutes
// incorrectly for the macro). Any file including platform.h directly or transitively (i.e. via
// util/logging.h) will have this problem when compiled on Windows (i.e. for unit tests)
#define NOMINMAX

#include "util/defines.h"
#include "util/logging.h"
#include "third_party/renderdoc/renderdoc_app.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

const RENDERDOC_API_1_0_0* GetRenderDocApi()
{
    GFXRECON_LOG_INFO("The RenderDoc In-Application API is only supported on Android");
    return nullptr;
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
