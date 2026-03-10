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

# Crashpad is only used in RelWithDebInfo builds when DIVE_BUILD_WITH_CRASHPAD is enabled.
# Otherwise, absl failure signal handler is used.
set(USE_CRASHPAD_CONDITION
    $<AND:$<CONFIG:RelWithDebInfo>,$<BOOL:${DIVE_BUILD_WITH_CRASHPAD}>>
)

add_library(crash_report STATIC)

target_sources(
    crash_report
    PRIVATE
        absl_crash_handler.h
        crash_report.h
        $<$<BOOL:${USE_CRASHPAD_CONDITION}>:crash_report_crashpad.cpp>
        $<$<NOT:${USE_CRASHPAD_CONDITION}>:crash_report_absl.cpp>
)

target_link_libraries(
    crash_report
    PUBLIC absl::status
    PRIVATE
        absl::debugging
        absl::symbolize
        absl::failure_signal_handler
        absl::strings
        $<$<BOOL:${USE_CRASHPAD_CONDITION}>:dive_crashpad>
)
