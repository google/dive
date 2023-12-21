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
    set(CMAKE_PREFIX_PATH_PRE ${CMAKE_PREFIX_PATH})
    # TODO(renfeng): add prebuild libarchive or remove this
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/prebuild/libarchive/")
    add_compile_definitions(LIBARCHIVE_STATIC)
endif()

find_package(LibArchive QUIET)

if(NOT LibArchive_FOUND)
    # For Windows we need to build the zlib
    if(WIN32)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH_PRE})
        # Enable find_package uses of <PackageName>_ROOT variables.
        cmake_policy(SET CMP0074 NEW)
        set(ZLIB_ROOT  ${THIRDPARTY_DIRECTORY}/grpc/third_party/zlib)
        if(EXISTS "${ZLIB_ROOT}/CMakeLists.txt")
            include_directories("${ZLIB_ROOT}")
            add_subdirectory(${ZLIB_ROOT} third_party/zlib)
            find_package(ZLIB)
            if(ZLIB_FOUND)
                SET(ZLIB_INCLUDE_DIR ${CMAKE_BINARY_DIR}/third_party/zlib ${ZLIB_INCLUDE_DIR})
                message("ZLIB_INCLUDE_DIR is " ${ZLIB_INCLUDE_DIR})
            else()
                message(FATAL_ERROR "did not find zlib")
            endif()
        endif()
    endif()

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


link_directories(LibArchive_LIBRARIES)
include_directories(LibArchive_INCLUDE_DIRS)
message("LibArchive_LIBRARIES is " ${LibArchive_LIBRARIES})
message("LibArchive_INCLUDE_DIRS is " ${LibArchive_INCLUDE_DIRS})

