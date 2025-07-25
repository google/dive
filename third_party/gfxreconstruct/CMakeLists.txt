###############################################################################
# Copyright (c) 2018-2023 LunarG, Inc.
# Copyright (c) 2019-2020 Advanced Micro Devices, Inc.
# All rights reserved
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Author: LunarG Team
# Author: AMD Developer Tools Team
# Description: CMake script for framework util target
###############################################################################
cmake_minimum_required(VERSION 3.16.3)
project(GFXReconstruct LANGUAGES CXX)

set_property(GLOBAL PROPERTY TEST_SCRIPT ${CMAKE_ROOT})

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${PROJECT_SOURCE_DIR}/external/cmake-modules")
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_VISIBILITY_INLINES_HIDDEN "YES")

option(USE_STRICT_COMPILER_WARNINGS "Use a stricter compiler-warning level" OFF)

if (USE_STRICT_COMPILER_WARNINGS)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wconversion -Wextra -Werror -Wshadow -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wconversion -Wextra -Werror -Wshadow -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers")
    else ()
        message(WARNING "option 'USE_STRICT_COMPILER_WARNINGS' is set, but not implemented for this platform")
    endif ()
endif (USE_STRICT_COMPILER_WARNINGS)

# Version info
set(GFXRECONSTRUCT_PROJECT_VERSION_MAJOR 1)
set(GFXRECONSTRUCT_PROJECT_VERSION_MINOR 0)
set(GFXRECONSTRUCT_PROJECT_VERSION_PATCH 5)

set(GFXRECON_PROJECT_VERSION_SHA1 "unknown-build-source")

option(BUILD_STATIC "Build static binaries for HPC clusters (will not build the tracer library)" OFF)

if (NOT DEFINED HEADLESS)
    set(HEADLESS TRUE)
endif ()

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_get_exact_tag(GIT_TAG)
set(GIT_BRANCH "")

if (GIT_REFSPEC)
    string(REGEX REPLACE ".*/(.+)$" "\\1" GIT_BRANCH ${GIT_REFSPEC})
    string(COMPARE EQUAL ${GIT_BRANCH} "master" GIT_IS_MASTER)
    string(REGEX MATCH "^vulkan-sdk-[0-9]+\.[0-9]+\.[0-9]+$" GIT_IS_SDK ${GIT_BRANCH})

    if (GIT_IS_MASTER OR GIT_IS_SDK)
        if (GIT_TAG)
            set(GIT_BRANCH ${GIT_TAG})
        else ()
            set(GIT_BRANCH "")
        endif ()
        if (NOT DEFINED GFXRECON_PROJECT_VERSION_DESIGNATION)
            set(GFXRECON_PROJECT_VERSION_DESIGNATION "")
        endif ()
    elseif (NOT DEFINED GFXRECON_PROJECT_VERSION_DESIGNATION)
        set(GFXRECON_PROJECT_VERSION_DESIGNATION "-dev")
    endif ()
elseif (GIT_TAG)
    string(REGEX MATCH "^v[0-9]+\.[0-9]+\.[0-9]+$" GIT_IS_VERSION_RELEASE_TAG ${GIT_TAG})
    if (GIT_IS_VERSION_RELEASE_TAG)
        set(GIT_BRANCH ${GIT_TAG})
        set(GFXRECON_PROJECT_VERSION_DESIGNATION "")
    endif ()
elseif (NOT DEFINED GFXRECON_PROJECT_VERSION_DESIGNATION)
    set(GFXRECON_PROJECT_VERSION_DESIGNATION "-unknown")
endif ()

if (GIT_SHA1)
    string(SUBSTRING ${GIT_SHA1} 0 7 GFXRECON_PROJECT_VERSION_SHA1)

    if (GIT_BRANCH)
        string(CONCAT GFXRECON_PROJECT_VERSION_SHA1 ${GIT_BRANCH} ":" ${GFXRECON_PROJECT_VERSION_SHA1})
    endif ()

    git_local_changes(GIT_LOCAL_STATE)
    string(COMPARE EQUAL ${GIT_LOCAL_STATE} "DIRTY" GIT_DIRTY)
    if (GIT_DIRTY)
        string(CONCAT GFXRECON_PROJECT_VERSION_SHA1 ${GFXRECON_PROJECT_VERSION_SHA1} "*")
    endif ()
endif ()

# Adds all the configure time information into project_version_temp.h.in
configure_file("${PROJECT_SOURCE_DIR}/project_version.h.in" "${CMAKE_BINARY_DIR}/project_version_temp.h.in")

# Generate a "project_version_$<CONFIG>.h" for the current config - necessary to determine the current build configuration
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/project_version_$<CONFIG>.h" INPUT "${CMAKE_BINARY_DIR}/project_version_temp.h.in")

# Since project_version_$<CONFIG>.h differs per build, set a compiler definition that files can use to include it
add_definitions(-DPROJECT_VERSION_HEADER_FILE="project_version_$<CONFIG>.h")

option(BUILD_WERROR "Build with warnings as errors" ON)

# Code checks
include("CodeStyle")
include("Lint")
include("Test")
include("FindVulkanVersion")

set(OPENXR_SUPPORT_ENABLED FALSE)

option(GFXRECON_ENABLE_OPENXR "Enable OpenXR support." ON)
if (GFXRECON_ENABLE_OPENXR)
    include("FindOpenXRVersion")

    if ((NOT XR_VERSION_MAJOR STREQUAL "") AND (NOT XR_VERSION_MINOR STREQUAL ""))
        set(OPENXR_SUPPORT_ENABLED TRUE)
        message(STATUS "OpenXR support enabled")
    else()
        message(FATAL_ERROR "Failed to find OpenXR headers for support!")
    endif()
else()
    message(STATUS "OpenXR support disabled!")
endif()

# Apply misc build directives to the given target
macro(common_build_directives TARGET)
    target_code_style_build_directives(${TARGET})
    target_lint_build_directives(${TARGET})
    add_target_to_test_package(${TARGET})
endmacro()

option(GFXRECON_ENABLE_RELEASE_ASSERTS "Enable release builds to output failed conditions caught by GFXRECON_ASSERT macro." OFF)
if (${GFXRECON_ENABLE_RELEASE_ASSERTS})
    add_definitions(-DGFXRECON_ENABLE_RELEASE_ASSERTS)
endif ()

option(GFXRECON_TOCPP_SUPPORT "Build ToCpp export tool as part of GFXReconstruct builds." TRUE)

option(GFXRECON_INCLUDE_TEST_APPS "Build and install test apps" OFF)

if (MSVC)

    # The host toolchain architecture (i.e. are the compiler and other tools compiled to ARM/Intel 32bit/64bit binaries):
    message(STATUS "CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE: " ${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE})
    # The platform name (architecture), we want to build our project for:
    # e.g. passed through the commandline -A option: cmake -G "Visual Studio 17 2022" -A ARM64
    message(STATUS "CMAKE_GENERATOR_PLATFORM: " ${CMAKE_GENERATOR_PLATFORM})

    message(STATUS "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION: " ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})

    set(GFXR_ARM_WINDOWS_BUILD FALSE)
    # Some IDEs pass -A in lowercase or in mixed case.
    # Normalize the CMAKE_GENERATOR_PLATFORM string to upper case to avoid architecture detection mismatch.
    string(TOUPPER "${CMAKE_GENERATOR_PLATFORM}" NORMALIZED_CMAKE_GENERATOR_PLATFORM)
    if (NORMALIZED_CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
        set(GFXR_ARM_WINDOWS_BUILD TRUE)
    endif ()

    # Default to using the precompiled LZ4 and ZLIB binaries for VisualStudio builds.
    set(PRECOMPILED_ARCH "64")
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(PRECOMPILED_ARCH "32")
    endif (CMAKE_SIZEOF_VOID_P EQUAL 4)

    if (GFXR_ARM_WINDOWS_BUILD)
        set(PRECOMPILED_ARCH "64-arm")
    endif (GFXR_ARM_WINDOWS_BUILD)

    set(CMAKE_PREFIX_PATH
            ${CMAKE_PREFIX_PATH}
            "${PROJECT_SOURCE_DIR}/external/precompiled/win${PRECOMPILED_ARCH}"
            "${PROJECT_SOURCE_DIR}/external/precompiled/win${PRECOMPILED_ARCH}")

    # Enable LARGEADDRESSAWARE to increase memory address space for 32-bit executables
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")

    # Add option to enable/disable D3D12 support.
    option(D3D12_SUPPORT "Build with Direct3D 12 support enabled" ON)

    # Add option to enable/disable launcher and interceptor.
    option(BUILD_LAUNCHER_AND_INTERCEPTOR "Build launcher and interceptor" OFF)

    # Enable STL locks to work after building capture libs with certain versions of VS
    add_definitions(-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)

    if (${D3D12_SUPPORT})
        set(D3D12_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/external/AgilitySDK/include")
        add_definitions(-DD3D12_SUPPORT)

        # Check Windows SDK version and print warning if there is a mismatch.
        set(EXPECTED_WIN_SDK_VER "10.0.20348.0")
        if (NOT ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} STREQUAL ${EXPECTED_WIN_SDK_VER})
            message(WARNING
                    "D3D12 support is authored against Windows SDK ${EXPECTED_WIN_SDK_VER}. Windows SDK version "
                    "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} may not be compatible. If you encounter build errors, "
                    "set D3D12_SUPPORT=OFF or configure the build with the recommended Windows SDK version. See BUILD.md "
                    "for more information.")
        endif ()

        find_package(DXC)
        if (DXC_FOUND)
            set(D3D12_INCLUDE_DIRECTORIES ${D3D12_INCLUDE_DIRECTORIES} ${DXC_INCLUDE_DIR})
            add_definitions(-DGFXRECON_DXC_SUPPORT)
        endif ()

    else ()
        set(BUILD_LAUNCHER_AND_INTERCEPTOR OFF)
    endif ()


    if (${BUILD_LAUNCHER_AND_INTERCEPTOR})
        add_definitions(-DBUILD_LAUNCHER_AND_INTERCEPTOR)
    endif ()

    if(NOT ${GFXR_ARM_WINDOWS_BUILD})
        # Add option to enable/disable AGS support. Only supported on non-ARM64 builds.
        option(GFXRECON_AGS_SUPPORT "Build with AGS support enabled. Not supported on Windows on ARM64. Ignored if D3D12_SUPPORT=OFF." ON)
    else()
        set(GFXRECON_AGS_SUPPORT OFF)
        message(STATUS "GFXRECON_AGS_SUPPORT is disabled for Windows on ARM64 builds.")
    endif()

    if (${D3D12_SUPPORT})
        if (${GFXRECON_AGS_SUPPORT})
            if (CMAKE_SIZEOF_VOID_P EQUAL 8)
                find_package(AGS)
                if (AGS_FOUND)
                    add_definitions(-DGFXRECON_AGS_SUPPORT)

                    set(D3D12_INCLUDE_DIRECTORIES ${D3D12_INCLUDE_DIRECTORIES} ${AGS_INCLUDE_DIR})

                    # The value for option GFXRECON_AGS_SUPPORT gets cached so use a non-cached variable
                    # to determine the final result.
                    set(GFXRECON_AGS_SUPPORT_FINAL ON)
                else ()
                    message(WARNING "GFXRECON_AGS_SUPPORT was requested but the required dependencies were not found. "
                            "AGS support will be disabled.")
                endif () # AGS_FOUND
            else ()
                message(WARNING "GFXRECON_AGS_SUPPORT was requested but is not supported. An x64 build is required. "
                        "AGS support will be disabled.")
            endif () # CMAKE_SIZEOF_VOID_P EQUAL 8
        endif () # GFXRECON_AGS_SUPPORT
    endif () # D3D12_SUPPORT

else (MSVC)
    # Turn off D3D12 support for non MSVC builds.
    set(D3D12_SUPPORT OFF)
    set(BUILD_LAUNCHER_AND_INTERCEPTOR OFF)
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(CMAKE_PREFIX_PATH
                ${CMAKE_PREFIX_PATH}
                "${PROJECT_SOURCE_DIR}/external/precompiled/linux/lib/arm64-v8a")
    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
        set(CMAKE_PREFIX_PATH
                ${CMAKE_PREFIX_PATH}
                "${PROJECT_SOURCE_DIR}/external/precompiled/linux/lib/arm")
    endif ()
endif (MSVC)

if (APPLE)
    add_link_options(-Wl,-dead_strip -Wl,-dead_strip_dylibs)
    set(CMAKE_PREFIX_PATH
            "${PROJECT_SOURCE_DIR}/external/precompiled/macos"
            ${CMAKE_PREFIX_PATH})
endif ()

# GFXReconstruct provided find modules
if (WIN32)
    find_package(Detours)
endif () # WIN32

option(LZ4_OPTIONAL "Allow building without LZ4-compression" OFF)
find_package(LZ4)

if (NOT LZ4_FOUND)
    if (LZ4_OPTIONAL)
        message(STATUS "LZ4 library not found, but declared optional. LZ4 support is disabled.")
    else ()
        message(FATAL_ERROR "LZ4 library not found. Please install LZ4 or allow building without it by setting LZ4_OPTIONAL to ON.")
    endif ()# LZ4_OPTIONAL
endif ()# NOT LZ4_FOUND

find_package(ZSTD)

# Find and use nlohmann_json
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann-json")
find_package(nlohmann_json REQUIRED)

if (UNIX AND NOT APPLE)
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" ON)
    option(BUILD_WSI_DISPLAY_SUPPORT "Build direct-to-display swapchain support" ON)

    if (BUILD_WSI_XCB_SUPPORT)
        find_package(XCB)
    endif ()

    if (BUILD_WSI_WAYLAND_SUPPORT)
        find_package(WAYLAND)
    endif ()
endif (UNIX AND NOT APPLE)

# CMake provided find modules
if (BUILD_STATIC)
    find_library(ZLIB NAMES libz.a REQUIRED PATH_SUFFIXES lib lib/x86_64-linux-gnu lib64)
else ()
    find_package(ZLIB)
endif ()
if (UNIX AND NOT APPLE)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    if (BUILD_WSI_XLIB_SUPPORT)
        find_package(X11)
    endif ()
endif (UNIX AND NOT APPLE)

add_library(windows_specific INTERFACE)

target_compile_definitions(windows_specific INTERFACE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        VK_USE_PLATFORM_WIN32_KHR
        XR_USE_PLATFORM_WIN32
        XR_USE_GRAPHICS_API_VULKAN
        $<$<BOOL:${HEADLESS}>:VK_USE_PLATFORM_HEADLESS>
        $<$<BOOL:${BUILD_LAUNCHER_AND_INTERCEPTOR}>:"GFXR_INTERCEPTOR_PATH=\"$<TARGET_FILE:gfxrecon_interceptor>\"">
        $<$<BOOL:${D3D12_SUPPORT}>:"GFXR_D3D12_CAPTURE_PATH=\"$<TARGET_FILE:d3d12_capture>\"">
        $<$<BOOL:${D3D12_SUPPORT}>:"GFXR_D3D12_PATH=\"$<TARGET_FILE:d3d12>\"">
        $<$<BOOL:${D3D12_SUPPORT}>:"GFXR_DXGI_PATH=\"$<TARGET_FILE:dxgi>\"">)


add_library(linux_specific INTERFACE)
target_compile_definitions(linux_specific INTERFACE _FILE_OFFSET_BITS=64 PAGE_GUARD_ENABLE_UCONTEXT_WRITE_DETECTION
        XR_USE_GRAPHICS_API_VULKAN
        $<$<BOOL:${X11_FOUND}>:VK_USE_PLATFORM_XLIB_KHR XR_USE_PLATFORM_XLIB>
        $<$<BOOL:${X11_Xrandr_FOUND}>:VK_USE_PLATFORM_XLIB_XRANDR_EXT>
        $<$<BOOL:${XCB_FOUND}>:VK_USE_PLATFORM_XCB_KHR XR_USE_PLATFORM_XCB>
        $<$<BOOL:${WAYLAND_FOUND}>:VK_USE_PLATFORM_WAYLAND_KHR XR_USE_PLATFORM_WAYLAND>
        $<$<BOOL:${BUILD_WSI_DISPLAY_SUPPORT}>:VK_USE_PLATFORM_DISPLAY_KHR>
        $<$<BOOL:${HEADLESS}>:VK_USE_PLATFORM_HEADLESS>)

add_library(macos_specific INTERFACE)
target_compile_definitions(macos_specific INTERFACE
        XR_USE_GRAPHICS_API_VULKAN
        VK_USE_PLATFORM_METAL_EXT
        $<$<BOOL:${HEADLESS}>:VK_USE_PLATFORM_HEADLESS>)

add_library(platform_specific INTERFACE)
target_link_libraries(platform_specific INTERFACE
        $<$<BOOL:${WIN32}>:windows_specific>
        $<$<BOOL:${APPLE}>:macos_specific>
        $<$<NOT:$<OR:$<BOOL:${WIN32}>,$<BOOL:${APPLE}>>>:linux_specific>)

if (BUILD_WERROR)
    if (MSVC)
        target_compile_options(platform_specific INTERFACE /WX)
    else ()
        target_compile_options(platform_specific INTERFACE -Werror)
    endif ()
endif ()

if(MSVC)
    target_compile_options(platform_specific INTERFACE /MP)
endif()

add_library(vulkan_registry INTERFACE)
target_include_directories(vulkan_registry SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include)
target_compile_definitions(vulkan_registry INTERFACE VK_NO_PROTOTYPES VK_ENABLE_BETA_EXTENSIONS)

add_library(spirv_registry INTERFACE)
target_include_directories(spirv_registry SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/external/SPIRV-Headers/include)

add_library(OpenXR INTERFACE)
if (OPENXR_SUPPORT_ENABLED)
    add_subdirectory(${PROJECT_SOURCE_DIR}/external/OpenXR-SDK)
    target_include_directories(OpenXR INTERFACE ${PROJECT_SOURCE_DIR}/external/OpenXR-SDK/include)
    target_compile_definitions(OpenXR INTERFACE XR_NO_PROTOTYPES ENABLE_OPENXR_SUPPORT=1)
endif()

add_library(vulkan_memory_allocator INTERFACE)
target_compile_options(vulkan_memory_allocator INTERFACE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wno-nullability-completeness>
)
target_include_directories(vulkan_memory_allocator SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/external/VulkanMemoryAllocator/include)

# SPIRV-Reflect included as submodule -> libspirv-reflect-static.a
set(SPIRV_REFLECT_EXAMPLES OFF)
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory(external/SPIRV-Reflect EXCLUDE_FROM_ALL)

if (${RUN_TESTS})
    add_library(catch2 INTERFACE)
    target_include_directories(catch2 SYSTEM INTERFACE external)
endif ()

add_subdirectory(framework)
if (NOT BUILD_STATIC)
    add_subdirectory(layer)
endif ()
add_subdirectory(tools)

if (GFXRECON_INCLUDE_TEST_APPS)
    add_subdirectory(external/Vulkan-Headers)
    add_subdirectory(test)
endif ()

if (${RUN_TESTS})
    add_test_package_file(${CMAKE_CURRENT_LIST_DIR}/scripts/build.py)
    add_test_package_file(${CMAKE_CURRENT_LIST_DIR}/scripts/test.py)
    generate_test_package(gfx_reconstruct_test)
endif ()

add_subdirectory(scripts)
