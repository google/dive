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

# Enables common compiler warnings. This is directory-based (not target-based) so that new targets
# also get the warnings enabled.
function(enable_dive_compiler_warnings)
    if(MSVC)
        # TOOD: b/509938195 - Remove /wd4100 after problems are fixed
        add_compile_options(
            /WX
            /W4
            /w44456 # warn if local shadows previous local
            /w44457 # warn if local shadows function parameter
            /w44458 # warn if local shadows class member
            /w44459 # warn if local shadows global variable
            /wd4201 # adreno.h uses non-standard nameless struct/union
            /wd4100 # emulate_pm4.h has unreferenced parameters
            /wd4127 # "conditional expression is constant" caused by Abseil
            /wd4324 # "structure was padded" caused by Abseil flags
            /w44062 # require exhastive switches
            /wd4251 # android_application.cc exports std::string (template member)
        )
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    else()
        add_compile_options(
            -Werror
            -Wall
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wshadow-all>
            $<$<CXX_COMPILER_ID:GNU>:-Wshadow>
            -Wswitch
            -Wextra
            # TODO: b/509938195 - Remove -Wno-unused-parameter when problems are fixed
            -Wno-unused-parameter
            # -Wmissing-field-initializers fights with readability-redundant-member-init.
            # Rely on clang-tidy to catch the important ommissions.
            -Wno-missing-field-initializers
        )
    endif()
endfunction()
