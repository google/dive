#
# Copyright 2023 Google LLC
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

# Freedreno can be compiled on its own; keep the version and project info.
cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(freedreno C CXX)

if(NOT ANDROID)
    message(FATAL_ERROR "Dive only supports libwrap for Android.")
endif()

add_library(freedreno_includes INTERFACE)
target_include_directories(freedreno_includes INTERFACE includes util)

add_subdirectory(wrap)
