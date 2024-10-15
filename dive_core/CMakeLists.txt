#
# Copyright 2019 Google LLC
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

cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(dive_core)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)

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
set(FREEDRENO_DEVICES_HDR_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/freedreno_devices.h")
set(FREEDRENO_IR3_ISA_SRC_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/ir3-isa.c")
set(FREEDRENO_IR3_ISA_HDR_FILE "${DIVE_CORE_GENERATED_FILE_DIR}/ir3-isa.h")


if(WIN32)
  set(Python3_FIND_REGISTRY "NEVER")
endif()
if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
  find_program (Python3_EXECUTABLE python3)
else()
  find_package (Python3 COMPONENTS Interpreter)
endif()

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

file(GLOB_RECURSE HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "_tests.cpp$")

if(DIVE_NO_DISASSEMBLY)
list(FILTER SRC_FILES EXCLUDE REGEX "shader_disassembly\\.cpp")
else()
list(FILTER SRC_FILES EXCLUDE REGEX "shader_disassembly_dummy\\.cpp")
endif()

file(GLOB_RECURSE COMMON_HDR_FILES "${CMAKE_SOURCE_DIR}/dive_core/common/*.h")
file(GLOB_RECURSE COMMON_SRC_FILES "${CMAKE_SOURCE_DIR}/dive_core/common/*.cpp")

file(GLOB_RECURSE FREEDRENO_SRC_FILES "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common/freedreno_dev_info.c")

include_directories(${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include ${CMAKE_SOURCE_DIR} ${DIVE_CORE_GENERATED_FILE_DIR})
include_directories(${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common)
include_directories(${THIRDPARTY_DIRECTORY}/mesa/src/)
include_directories(${THIRDPARTY_DIRECTORY}/mesa/include/)
include_directories(${THIRDPARTY_DIRECTORY}/libarchive/libarchive/)


add_definitions(-DLITTLEENDIAN_CPU)     # Required for some PAL files

add_library(${PROJECT_NAME} STATIC ${HDR_FILES} ${SRC_FILES}
  ${COMMON_HDR_FILES} ${COMMON_SRC_FILES} ${PM4_GENERATED_SRC_FILE} ${NATVIS_FILES} ${FREEDRENO_SRC_FILES})
add_dependencies(${PROJECT_NAME} pm4_info)
add_dependencies(${PROJECT_NAME} adreno_header)
add_dependencies(${PROJECT_NAME} a6xx_py)
add_dependencies(${PROJECT_NAME} freedreno_devices_header)



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
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/half_float.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/hash_table.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/google_dive_ext.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/log.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/os_file.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/os_misc.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/ralloc.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/simple_mtx.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/softfloat.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_call_once.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_debug.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_printf.c"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/u_process.c"
)

target_include_directories(freedreno_ir3 PRIVATE
    "${THIRDPARTY_DIRECTORY}/mesa/include"
    "${THIRDPARTY_DIRECTORY}/mesa/src"
    "${THIRDPARTY_DIRECTORY}/mesa/src/compiler"
    "${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec"
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno"
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common"
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/isa"
    "${THIRDPARTY_DIRECTORY}/mesa/src/util/ralloc.c"
)

if(MSVC)
target_compile_definitions(freedreno_ir3 PRIVATE
    -DHAVE_STRUCT_TIMESPEC
)
else()
target_compile_definitions(freedreno_ir3 PRIVATE
    -DHAVE_ENDIAN_H
    -DHAVE_FUNC_ATTRIBUTE_PACKED
    -DHAVE_STRUCT_TIMESPEC
    -DHAVE_PTHREAD
)
endif()

target_compile_options(freedreno_ir3 PRIVATE -fPIC)
add_dependencies(${PROJECT_NAME} freedreno_ir3)
target_link_libraries(${PROJECT_NAME} PRIVATE freedreno_ir3)


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

include_directories(${LibArchive_INCLUDE_DIRS} 
  "${THIRDPARTY_DIRECTORY}/perfetto/" 
  "${THIRDPARTY_DIRECTORY}/perfetto/include"
  ${PERFETTO_BUILD_HEADER_DIR})
if (MSVC)
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  # 4127: conditional expression is constant
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4100 /wd4201 /wd4127)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces -Wno-sign-compare -fPIC
    $<$<COMPILE_LANGUAGE:C>:-Wno-override-init>
  )
endif()
