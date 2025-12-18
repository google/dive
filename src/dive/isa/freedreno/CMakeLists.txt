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

message(CHECK_START "Generate build files for src/freedreno")
if(ANDROID)
    message(CHECK_FAIL "detected Android platform, skipping")
    return()
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(DIVE_FREEDRENO_GENFILE_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(DIVE_FREEDRENO_GENFILE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated/dive/isa/freedreno"
)
set(PM4_INFO_FILE "${DIVE_FREEDRENO_GENFILE_DIR}/pm4_info")
set(PM4_GENERATED_SRC_FILE "${DIVE_FREEDRENO_GENFILE_DIR}/pm4_info.cpp")
set(PM4_GENERATED_HDR_FILE "${DIVE_FREEDRENO_GENFILE_DIR}/pm4_info.h")
set(ADRENO_HDR_FILE "${DIVE_FREEDRENO_GENFILE_DIR}/adreno.h")
set(A6XX_PY_FILE "${DIVE_FREEDRENO_GENFILE_DIR}/a6xx.py")
set(FREEDRENO_ISA_DIRECTORY "${DIVE_FREEDRENO_GENFILE_DIR}/freedreno/isa")
set(FREEDRENO_DEVICES_HDR_FILE "${FREEDRENO_ISA_DIRECTORY}/freedreno_devices.h")
set(FREEDRENO_IR3_ISA_SRC_FILE "${FREEDRENO_ISA_DIRECTORY}/ir3-isa.c")
set(FREEDRENO_IR3_ISA_HDR_FILE "${FREEDRENO_ISA_DIRECTORY}/ir3-isa.h")

if(WIN32)
    set(Python3_FIND_REGISTRY "NEVER")
endif() # WIN32
find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "python3 not found")
endif() # NOT Python3_EXECUTABLE

add_custom_command(
    OUTPUT ${PM4_GENERATED_HDR_FILE} ${PM4_GENERATED_SRC_FILE}
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/generatePm4Info_adreno.py
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers" "adreno/a6xx.xml"
        ${PM4_INFO_FILE}
    DEPENDS a6xx_py "${CMAKE_CURRENT_SOURCE_DIR}/generatePm4Info_adreno.py"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)
add_custom_target(
    pm4_info
    ALL
    DEPENDS ${PM4_GENERATED_HDR_FILE} ${PM4_GENERATED_SRC_FILE}
)

add_custom_command(
    OUTPUT ${ADRENO_HDR_FILE}
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/generateAdrenoHeader.py
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers" "adreno/a6xx.xml"
        ${ADRENO_HDR_FILE}
    DEPENDS a6xx_py "${CMAKE_CURRENT_SOURCE_DIR}/generateAdrenoHeader.py"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)
add_custom_target(adreno_header ALL DEPENDS ${ADRENO_HDR_FILE})

add_custom_command(
    OUTPUT ${A6XX_PY_FILE}
    COMMAND
        ${Python3_EXECUTABLE}
        ${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers/gen_header.py
        "--rnn" "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers" "--xml"
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers/adreno/a6xx.xml"
        "py-defines" > ${A6XX_PY_FILE}
    DEPENDS "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers/gen_header.py"
    WORKING_DIRECTORY ${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/registers
    VERBATIM
)
add_custom_target(a6xx_py ALL DEPENDS ${A6XX_PY_FILE})

add_custom_command(
    OUTPUT ${FREEDRENO_DEVICES_HDR_FILE}
    COMMAND
        ${Python3_EXECUTABLE}
        ${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common/freedreno_devices.py
        "-p" "${DIVE_CORE_GENERATED_FILE_DIR}" > ${FREEDRENO_DEVICES_HDR_FILE}
    DEPENDS
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common/freedreno_devices.py"
        "${A6XX_PY_FILE}"
    WORKING_DIRECTORY "${DIVE_FREEDRENO_GENFILE_DIR}"
    VERBATIM
)
add_custom_target(
    freedreno_devices_header
    ALL
    DEPENDS ${FREEDRENO_DEVICES_HDR_FILE}
)

if(MSVC)
    set_target_properties(
        freedreno_devices_header
        PROPERTIES FOLDER "freedreno"
    )
endif() # MSVC

set(FREEDRENO_SRC_FILES
    "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common/freedreno_dev_info.c"
)

if(NOT MSVC AND NOT APPLE)
    add_definitions(-DHAVE_ENDIAN_H) # Required for freedreno_dev_info.c on Linux
endif() # NOT MSVC AND NOT APPLE

make_directory("${FREEDRENO_ISA_DIRECTORY}")

add_custom_command(
    OUTPUT ${FREEDRENO_IR3_ISA_SRC_FILE} ${FREEDRENO_IR3_ISA_HDR_FILE}
    COMMAND
        ${Python3_EXECUTABLE}
        ${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec/decode.py "--xml"
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/isa/ir3.xml" "--out-c"
        ${FREEDRENO_IR3_ISA_SRC_FILE} "--out-h" ${FREEDRENO_IR3_ISA_HDR_FILE}
    DEPENDS
        "${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec/decode.py"
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/isa/ir3.xml"
    VERBATIM
)

add_library(
    freedreno_ir3
    STATIC
    ${FREEDRENO_IR3_ISA_SRC_FILE}
    ${FREEDRENO_IR3_ISA_HDR_FILE}
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
add_dependencies(freedreno_ir3 freedreno_devices_header)

target_compile_options(freedreno_ir3 PRIVATE -Wno-initializer-overrides)

target_include_directories(
    freedreno_ir3
    PRIVATE
        "${DIVE_FREEDRENO_GENFILE_DIR}"
        "${THIRDPARTY_DIRECTORY}/mesa/include"
        "${THIRDPARTY_DIRECTORY}/mesa/src"
        "${THIRDPARTY_DIRECTORY}/mesa/src/compiler"
        "${THIRDPARTY_DIRECTORY}/mesa/src/compiler/isaspec"
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno"
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common"
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/isa"
)

if(MSVC)
    target_compile_definitions(freedreno_ir3 PRIVATE -DHAVE_STRUCT_TIMESPEC)
    set_property(TARGET freedreno_ir3 PROPERTY C_STANDARD 11) # To support alignas
    target_sources(
        freedreno_ir3
        PRIVATE "${THIRDPARTY_DIRECTORY}/mesa/src/util/google_dive_ext.c"
    )
    set_target_properties(freedreno_ir3 PROPERTIES FOLDER "freedreno")
elseif(UNIX AND NOT APPLE)
    #set_source_files_properties(PROPERTIES LANGUAGE CXX
    target_compile_definitions(
        freedreno_ir3
        PRIVATE
            -DHAVE_ENDIAN_H
            -DHAVE_FUNC_ATTRIBUTE_PACKED
            -DHAVE_STRUCT_TIMESPEC
            -DHAVE_PTHREAD
            -DHAVE_SYSCONF
            -D_GNU_SOURCE
            -DHAVE_SECURE_GETENV
            -DHAVE_PROGRAM_INVOCATION_NAME
    )
elseif(APPLE)
    target_compile_definitions(
        freedreno_ir3
        PRIVATE
            -DHAVE_FUNC_ATTRIBUTE_PACKED
            -DHAVE_STRUCT_TIMESPEC
            -DHAVE_PTHREAD
            -DHAVE_SYSCONF
            -D_GNU_SOURCE
            -DHAVE_PROGRAM_INVOCATION_NAME
    )
    target_sources(
        freedreno_ir3
        PRIVATE
            "${THIRDPARTY_DIRECTORY}/mesa/src/c11/impl/threads_posix.c"
            "${THIRDPARTY_DIRECTORY}/mesa/src/c11/impl/time.c"
    )
endif() # MSVC

target_compile_options(freedreno_ir3 PRIVATE -fPIC)

add_library(
    dive_freedreno
    STATIC
    ${PM4_GENERATED_SRC_FILE}
    ${FREEDRENO_SRC_FILES}
)

target_link_libraries(dive_freedreno PUBLIC freedreno_ir3 dive_legacy_includes)

target_include_directories(
    dive_freedreno
    PRIVATE
        "${DIVE_FREEDRENO_GENFILE_DIR}"
        "${FREEDRENO_ISA_DIRECTORY}"
        "${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include"
        "${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common"
        "${THIRDPARTY_DIRECTORY}/libarchive/libarchive/"
    PUBLIC
        "${DIVE_FREEDRENO_GENFILE_INCLUDE_DIR}"
        "${THIRDPARTY_DIRECTORY}"
        "${THIRDPARTY_DIRECTORY}/mesa/src/"
        "${THIRDPARTY_DIRECTORY}/mesa/include/"
)

target_compile_options(dive_freedreno PRIVATE -Wno-initializer-overrides)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
