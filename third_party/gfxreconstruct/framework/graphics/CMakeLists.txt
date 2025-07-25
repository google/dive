###############################################################################
# Copyright (c) 2021 LunarG, Inc.
# Copyright (c) 2025 Advanced Micro Devices, Inc.
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
# Description: CMake script for framework graphics target
###############################################################################

add_library(gfxrecon_graphics STATIC "")

# GOOGLE: Add folder organization for windows build in dive.
if(MSVC)
    target_compile_options(gfxrecon_graphics PRIVATE /wd4996 /wd4267 /wd4244)
    set_target_properties(gfxrecon_graphics PROPERTIES FOLDER "gfxreconstruct")
endif()

target_sources(gfxrecon_graphics
               PRIVATE
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_gpu_va_map.h>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_gpu_va_map.cpp>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_resource_data_util.h>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_resource_data_util.cpp>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_util.h>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_util.cpp>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_shader_id_map.h>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_shader_id_map.cpp>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_image_renderer.h>
                    $<$<BOOL:${D3D12_SUPPORT}>:${CMAKE_CURRENT_LIST_DIR}/dx12_image_renderer.cpp>
                    ${CMAKE_CURRENT_LIST_DIR}/fps_info.h
                    ${CMAKE_CURRENT_LIST_DIR}/fps_info.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_check_buffer_references.h
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_check_buffer_references.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_resources_util.h
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_resources_util.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_device_util.h
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_device_util.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_instance_util.h
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_instance_util.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_util.h
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_util.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_struct_deep_copy.h
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_struct_get_pnext.h
                    ${CMAKE_CURRENT_LIST_DIR}/../generated/generated_vulkan_struct_deep_copy.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/../generated/generated_vulkan_struct_deep_copy_stype.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_struct_extract_handles.h
                    ${CMAKE_CURRENT_LIST_DIR}/vulkan_struct_extract_handles.cpp
                    ${CMAKE_CURRENT_LIST_DIR}/../generated/generated_vulkan_dispatch_table.h
              )

if (${GFXRECON_AGS_SUPPORT_FINAL})
target_sources(gfxrecon_graphics
               PRIVATE
                    ${CMAKE_CURRENT_LIST_DIR}/dx12_ags_marker_injector.h
                    ${CMAKE_CURRENT_LIST_DIR}/dx12_ags_marker_injector.cpp
               )
endif()

target_include_directories(gfxrecon_graphics
                           PUBLIC
                               ${PROJECT_SOURCE_DIR}/build
                               ${PROJECT_SOURCE_DIR}/framework
                               ${PROJECT_SOURCE_DIR}/external
                               $<$<BOOL:${D3D12_SUPPORT}>:${D3D12_INCLUDE_DIRECTORIES}>)

target_link_libraries(gfxrecon_graphics gfxrecon_util vulkan_registry platform_specific)

common_build_directives(gfxrecon_graphics)

if (${RUN_TESTS})
    add_executable(gfxrecon_graphics_test "")
    target_sources(gfxrecon_graphics_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/test/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../tools/platform_debug_helper.cpp)
    target_link_libraries(gfxrecon_graphics_test PRIVATE gfxrecon_graphics)
    if (MSVC)
        # Force inclusion of "gfxrecon_disable_popup_result" variable in linking.
        # On 32-bit windows, MSVC prefixes symbols with "_" but on 64-bit windows it doesn't.
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_link_options(gfxrecon_graphics_test PUBLIC "LINKER:/Include:_gfxrecon_disable_popup_result")
        else()
            target_link_options(gfxrecon_graphics_test PUBLIC "LINKER:/Include:gfxrecon_disable_popup_result")
        endif()
    endif()
    common_build_directives(gfxrecon_graphics_test)
    common_test_directives(gfxrecon_graphics_test)
endif()
