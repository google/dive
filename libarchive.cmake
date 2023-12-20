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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

if(WIN32)
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/prebuild/libarchive/")   
endif()

find_package(LibArchive QUIET)

add_compile_definitions(LIBARCHIVE_STATIC)

if(NOT LibArchive_FOUND)
    if(WIN32)
        message(FATAL_ERROR "Can not find libarchive, please download it from https://libarchive.org/downloads/ and put it under prebuild folder")
    else()
        set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_TEST OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_OPENSSL OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_LIBB2 OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_LZ4 OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_LZMA OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_ZSTD OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_BZip2 OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_CNG OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_TAR OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_CPIO OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_CAT OFF CACHE INTERNAL "" FORCE)
        set(ENABLE_ACL OFF CACHE INTERNAL "" FORCE)
        add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/libarchive)
        set(LibArchive_LIBRARIES archive_static)
        set(LibArchive_INCLUDE_DIRS ${THIRDPARTY_DIRECTORY}/libarchive/libarchive/)
        add_compile_definitions(LIBARCHIVE_STATIC=ON)
        include_directories(${THIRDPARTY_DIRECTORY}/libarchive/libarchive/) 
    endif()
endif()


link_directories(LibArchive_LIBRARIES)
include_directories(LibArchive_INCLUDE_DIRS)
message("LibArchive_LIBRARIES is " ${LibArchive_LIBRARIES})
message("LibArchive_INCLUDE_DIRS is " ${LibArchive_INCLUDE_DIRS})

