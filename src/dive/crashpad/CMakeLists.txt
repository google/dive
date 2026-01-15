#
# Copyright 2026 Google LLC
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

add_library(
    dive_crashpad
    crashpad_client.h
    $<IF:$<BOOL:${DIVE_BUILD_WITH_CRASHPAD}>,crashpad_client.cpp,crashpad_client_stub.cpp>
    crashpad_utils.h
    $<$<PLATFORM_ID:Windows>:crashpad_utils_win32.cpp>
    $<$<PLATFORM_ID:Darwin>:crashpad_utils_macos.cpp>
    $<$<PLATFORM_ID:Linux>:crashpad_utils_linux.cpp>
)

target_compile_definitions(
    dive_crashpad
    PRIVATE $<$<PLATFORM_ID:Windows>:UNICODE> $<$<PLATFORM_ID:Windows>:_UNICODE>
)

target_link_libraries(
    dive_crashpad
    PUBLIC dive_src_includes absl::status
    PRIVATE
        absl::base
        absl::statusor
        absl::strings
        version_info
        command_utils
        crashpad_client
)
