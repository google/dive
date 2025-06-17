#
# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

add_library(gfxr_dump_resources_lib STATIC gfxr_dump_resources.cpp state_machine.cpp states.cpp dump_resources_builder_consumer.cpp)
# For third_party includes, allow using the full path: #include "third_party/gfxreconstruct/framework/decode/file_processor.h"
# GFXR doesn't need to be PUBLIC since the tool's "public" headers don't expose GFXR constructs
target_link_libraries(gfxr_dump_resources_lib PRIVATE gfxr_decode_ext_lib)

add_executable(gfxr_dump_resources main.cpp)
target_link_libraries(gfxr_dump_resources gfxr_dump_resources_lib)
