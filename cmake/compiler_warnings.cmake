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

function(enable_warnings_as_errors)
    if(MSVC)
        add_compile_options(/WX /W4)
    else()
        add_compile_options(-Werror -Wall -Wextra)
    endif()
endfunction()

function(enable_dive_compiler_warnings)
    enable_warnings_as_errors()
    if(MSVC)
        # TOOD: b/509938195 - Remove /wd4100 after problems are fixed
        add_compile_options(
            /w44456 # warn if local shadows previous local
            /w44457 # warn if local shadows function parameter
            /w44458 # warn if local shadows class member
            /w44459 # warn if local shadows global variable
            /w44062 # warn for non-exhaustive switch
            /wd4201 # adreno.h uses non-standard nameless struct/union
            /wd4100 # emulate_pm4.h has unreferenced parameters
            /wd4127 # "conditional expression is constant" caused by Abseil
            /wd4324 # "structure was padded" caused by Abseil flags
        )
    else()
        add_compile_options(
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wshadow-all>
            $<$<CXX_COMPILER_ID:GNU>:-Wshadow>
            -Wswitch
        )
    endif()
endfunction()
