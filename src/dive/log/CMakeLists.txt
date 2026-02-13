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

# === dive_android_log_sink ====================================================

if(ANDROID)
    add_library(dive_android_log_sink android_log_sink.h android_log_sink.cpp)
    target_link_libraries(
        dive_android_log_sink
        PRIVATE dive_src_includes absl::log absl::status -llog
    )
else()
    add_library(
        dive_android_log_sink
        android_log_sink.h
        android_log_sink_stub.cpp
    )
    target_link_libraries(
        dive_android_log_sink
        PRIVATE dive_src_includes absl::log absl::status
    )
endif()

# === dive_log =================================================================

add_library(dive_log log.h log.cpp)
target_link_libraries(
    dive_log
    PRIVATE
        dive_src_includes
        absl::log
        absl::log_initialize
        dive_android_log_sink
)

# === dive_log_utils ===========================================================

add_library(dive_log_utils log_utils.h log_utils.cpp)
target_link_libraries(
    dive_log_utils
    PRIVATE dive_src_includes absl::log
    PUBLIC absl::cleanup absl::statusor absl::strings
)
