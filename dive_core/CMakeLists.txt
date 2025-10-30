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

project(dive_core)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  file(GLOB_RECURSE NATVIS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.natvis")
else()
  set(NATVIS_FILES "")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(DIVE_CORE_GENERATED_FILE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(PM4_INFO_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/pm4_info")
set(PM4_GENERATED_SRC_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/pm4_info.cpp")
set(PM4_GENERATED_HDR_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/pm4_info.h")
set(ADRENO_HDR_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/adreno.h")
set(A6XX_PY_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/a6xx.py")
set(FREEDRENO_ISA_DIRECTORY "${DIVE_CORE_GENERATED_FILE_DIR}/freedreno/isa")
set(FREEDRENO_DEVICES_HDR_FILE "${FREEDRENO_ISA_DIRECTORY}/freedreno_devices.h")
set(FREEDRENO_IR3_ISA_SRC_FILE "${FREEDRENO_ISA_DIRECTORY}/ir3-isa.c")
set(FREEDRENO_IR3_ISA_HDR_FILE "${FREEDRENO_ISA_DIRECTORY}/ir3-isa.h")
set(TRACE_STATS_SRC_FILE "${CMAKE_SOURCE_DIR}/trace_stats/trace_stats.cpp")



if(WIN32)
  set(Python3_FIND_REGISTRY "NEVER")
endif()
# XXX XXX XXX  DO NOT SUBMIT
find_program (Python3_EXECUTABLE python3)

if (NOT Python3_EXECUTABLE)
  message(FATAL_ERROR "python3 not found")
endif()

add_custom_command(
    OUTPUT ${PM4_GENERATED_HDR_FILE} ${PM4_GENERATED_SRC_FILE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generatePm4Info_adreno.py
                   "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers"
                   "adreno/a6xx.xml"
                   ${PM4_INFO_FILE}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generatePm4Info_adreno.py"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)
add_custom_target(pm4_info ALL DEPENDS ${PM4_GENERATED_HDR_FILE} ${PM4_GENERATED_SRC_FILE})

add_custom_command(
    OUTPUT ${ADRENO_HDR_FILE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generateAdrenoHeader.py
                   "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers"
                   "adreno/a6xx.xml"
                   ${ADRENO_HDR_FILE}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generateAdrenoHeader.py"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)
add_custom_target(adreno_header ALL DEPENDS ${ADRENO_HDR_FILE})

add_custom_command(
    OUTPUT ${A6XX_PY_FILE}
    COMMAND ${Python3_EXECUTABLE} ${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers/gen_header.py
                   "--rnn"
                   "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers"
                   "--xml"
                   "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers/adreno/a6xx.xml"
                   "py-defines"
                   > ${A6XX_PY_FILE}
    DEPENDS "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers/gen_header.py"
    WORKING_DIRECTORY ${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers
    VERBATIM
)
add_custom_target(a6xx_py ALL DEPENDS ${A6XX_PY_FILE})

add_custom_command(
    OUTPUT ${FREEDRENO_DEVICES_HDR_FILE}
    COMMAND ${Python3_EXECUTABLE} ${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common/freedreno_devices.py
                   "-p"
                   "${DIVE_CORE_GENERATED_FILE_DIR}"
                   > ${FREEDRENO_DEVICES_HDR_FILE}
    DEPENDS "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common/freedreno_devices.py"
            "${A6XX_PY_FILE}"
    VERBATIM
)
add_custom_target(freedreno_devices_header ALL DEPENDS ${FREEDRENO_DEVICES_HDR_FILE})

if(MSVC)
    set_target_properties(freedreno_devices_header PROPERTIES FOLDER "freedreno")
endif()

file(GLOB_RECURSE HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "_tests.cpp$")
list(FILTER SRC_FILES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/tests/.*")

if(DIVE_NO_DISASSEMBLY)
list(FILTER SRC_FILES EXCLUDE REGEX "shader_disassembly\\.cpp")
else()
list(FILTER SRC_FILES EXCLUDE REGEX "shader_disassembly_dummy\\.cpp")
endif()

file(GLOB_RECURSE COMMON_HDR_FILES "${CMAKE_SOURCE_DIR}/dive_core/common/*.h")
file(GLOB_RECURSE COMMON_SRC_FILES "${CMAKE_SOURCE_DIR}/dive_core/common/*.cpp")

file(GLOB_RECURSE FREEDRENO_SRC_FILES "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common/freedreno_dev_info.c")

include_directories(${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include ${CMAKE_SOURCE_DIR} ${DIVE_CORE_GENERATED_FILE_DIR} ${FREEDRENO_ISA_DIRECTORY})
include_directories(${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common)
include_directories(${THIRDPARTY_DIRECTORY}/mesa/src/)
include_directories(${THIRDPARTY_DIRECTORY}/mesa/include/)
include_directories(${THIRDPARTY_DIRECTORY}/libarchive/libarchive/)

if(NOT MSVC)
  add_definitions(-DHAVE_ENDIAN_H)     # Required for freedreno_dev_info.c on Linux
endif()

add_library(${PROJECT_NAME} STATIC ${HDR_FILES} ${SRC_FILES}
  ${COMMON_HDR_FILES} ${COMMON_SRC_FILES} ${PM4_GENERATED_SRC_FILE} ${NATVIS_FILES} ${FREEDRENO_SRC_FILES} ${TRACE_STATS_SRC_FILE})
add_dependencies(${PROJECT_NAME} pm4_info)
add_dependencies(${PROJECT_NAME} adreno_header)
add_dependencies(${PROJECT_NAME} a6xx_py)
add_dependencies(${PROJECT_NAME} freedreno_devices_header)

make_directory("${FREEDRENO_ISA_DIRECTORY}")
add_custom_command(
      OUTPUT ${FREEDRENO_IR3_ISA_SRC_FILE} ${FREEDRENO_IR3_ISA_HDR_FILE}
      COMMAND ${Python3_EXECUTABLE} ${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec/decode.py
                    "--xml"
                    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/isa/ir3.xml"
                    "--out-c" ${FREEDRENO_IR3_ISA_SRC_FILE}
                    "--out-h" ${FREEDRENO_IR3_ISA_HDR_FILE}
      DEPENDS "${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec/decode.py"
              "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/isa/ir3.xml"
      VERBATIM
)

add_library(freedreno_ir3 STATIC
    ${FREEDRENO_IR3_ISA_SRC_FILE}
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/ir3/disasm-a3xx.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec/isaspec.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/blob.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/half_float.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/hash_table.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/log.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/os_file.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/os_misc.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/ralloc.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/simple_mtx.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/softfloat.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_call_once.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_cpu_detect.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_debug.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_printf.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_process.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_thread.c"
)

target_include_directories(freedreno_ir3 PRIVATE
    "${THIRDPARTY_DIRECTORY}/mesa/include"
    "${THIRDPARTY_DIRECTORY}/mesa/src"
    "${THIRDPARTY_DIRECTORY}/mesa/src/compiler"
    "${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec"
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno"
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common"
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/isa"
)

if(MSVC)
  target_compile_definitions(freedreno_ir3 PRIVATE
      -DHAVE_STRUCT_TIMESPEC
  )
  set_property(TARGET freedreno_ir3 PROPERTY C_STANDARD 11) # To support alignas
  target_sources(freedreno_ir3 PRIVATE "${THIRDPARTY_DIRECTORY}/mesa/src/util/google_dive_ext.c")
  set_target_properties(freedreno_ir3 PROPERTIES FOLDER "freedreno")
else()
#set_source_files_properties(PROPERTIES LANGUAGE CXX
  target_compile_definitions(freedreno_ir3 PRIVATE
      -DHAVE_ENDIAN_H
      -DHAVE_FUNC_ATTRIBUTE_PACKED
      -DHAVE_STRUCT_TIMESPEC
      -DHAVE_PTHREAD
      -DHAVE_SYSCONF
      -D_GNU_SOURCE
      -DHAVE_SECURE_GETENV
  )
endif()

target_compile_options(freedreno_ir3 PRIVATE -fPIC)
add_dependencies(${PROJECT_NAME} freedreno_ir3)
target_link_libraries(${PROJECT_NAME} PRIVATE freedreno_ir3)

# This is linked privately because gfxr contains X11 headers that contain preprocessor macros which cause collisions
# gfxr_decode_ext_lib itself can't link gfxrecon_decode privately because it's a dependency of a target (gfxrecon-replay) that doesn't specify PRIVATE/PUBLIC
target_link_libraries(${PROJECT_NAME} PRIVATE
    gfxr_decode_ext_lib
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/gfxreconstruct/framework
)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  target_link_libraries(${PROJECT_NAME} PRIVATE dl)
  target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
  target_link_libraries(${PROJECT_NAME} PRIVATE z)
  target_link_libraries(${PROJECT_NAME} PRIVATE tinfo)
endif()
if(WIN32)
  # Supress warning about deprecation of std::iterator when compile with c++17
  add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES} -static)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE ${LibArchive_LIBRARIES} ${PEFFETTO_TRACE_READER_LIB})

# This is needed in u_debug.h in mesa, which is included by shader_disassembly.cpp
check_function_exists(secure_getenv HAVE_SECURE_GETENV)
if (HAVE_SECURE_GETENV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DHAVE_SECURE_GETENV)
endif()

include_directories(${LibArchive_INCLUDE_DIRS})
if (MSVC)
  set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11) # To support alignas
  set_source_files_properties(${FREEDRENO_SRC_FILES} PROPERTIES COMPILE_FLAGS "/WX-")
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  # 4127: conditional expression is constant
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4100 /wd4201 /wd4127)
  # Add NOMINMAX definition for Windows builds to prevent min/max macro conflicts
  target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces -Wno-sign-compare -Wno-ignored-qualifiers -fPIC
    $<$<COMPILE_LANGUAGE:C>:-Wno-override-init>
  )
endif()

enable_testing()
add_subdirectory(tests)
