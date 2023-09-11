cmake_minimum_required(VERSION 3.0)

project(diveCaptureLayer)

set(CMAKE_CXX_STANDARD 17)
 

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


set(target_name VkLayer_dive_capture)

set(HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/dispatch.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_impl.h"
)

set(SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/dispatch.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_base.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_impl.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_impl.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_layer_android.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/gles_layer.cc"
)

include_directories(${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include)

add_library(${target_name} SHARED ${HDR_FILES}
                                ${SRC_FILES})
            
add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
add_definitions(-DLAYERNAME=VK_LAYER_DiveXr)                         

if(ANDROID)
target_link_libraries(${target_name}
                        PRIVATE -lm
                        PRIVATE -ldl
                        PRIVATE -llog
                        PRIVATE -landroid)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${target_name} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

if (MSVC)
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  target_compile_options(${target_name} PRIVATE /W4 /WX /wd4100 /wd4201)
else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces)
endif()

set_target_properties(${target_name}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)

