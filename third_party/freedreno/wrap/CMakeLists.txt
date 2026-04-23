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
    wrap
    SHARED
    dive-load-status.cc
    dive-wrap.c
    dive-wrap.h
    list.h
    wrap-syscall.c
    wrap-util.c
    wrap.h
)
target_link_libraries(wrap PRIVATE freedreno_includes log z)
# TODO: b/505831929 - Figure out which options are required
# _FORTIFY_SOURCE can likely go in target_compile_definitions however it was
# unclear to me if the #undef is necessary and whether or not we need to ensure
# it is first on the command-line.
target_compile_options(
    wrap
    PRIVATE -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
)
target_compile_definitions(
    wrap
    PRIVATE BIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD ANDROID_STL=c++_shared BIONIC=1
)

set_target_properties(
    wrap
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
        FOLDER "freedreno"
)

install(TARGETS wrap DESTINATION ${CMAKE_INSTALL_PREFIX})
