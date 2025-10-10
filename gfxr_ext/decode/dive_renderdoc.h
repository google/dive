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

#ifndef GFXRECON_DECODE_DIVE_RENDERDOC_H
#define GFXRECON_DECODE_DIVE_RENDERDOC_H

#include "util/defines.h"
#include "third_party/renderdoc/renderdoc_app.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

// Find and load the RenderDoc In-Application API. Returns nullptr on failure. The program or app
// must be launched with the RenderDoc capture layer. It is unclear how thread safe the API is so
// assume that it is thread-unsafe.
const RENDERDOC_API_1_0_0* GetRenderDocApi();

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif  // GFXRECON_DECODE_DIVE_RENDERDOC_H
