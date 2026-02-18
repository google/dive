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

if(ANDROID)
    message(
        CHECK_FAIL
        "breakpad tools including dump_syms and sym_upload are not targeted for Android."
    )
    return()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Locate dump_syms (External Dependency)
# We expect the developer to have installed this via Rust (cargo install dump_syms)
# or another method, and it must be available in the system PATH.

find_program(
    DUMP_SYMS
    NAMES dump_syms dump_syms.exe
    DOC "Path to the dump_syms executable"
)

if(DUMP_SYMS)
    message(STATUS "Found dump_syms: ${DUMP_SYMS}")
    add_executable(dump_syms IMPORTED GLOBAL)
    set_property(TARGET dump_syms PROPERTY IMPORTED_LOCATION "${DUMP_SYMS}")
else()
    message(
        FATAL_ERROR
        "Could not find 'dump_syms' in your PATH.\n"
        "Please install it before proceeding. The recommended way is via Rust:\n"
        "  cargo install dump_syms\n"
    )
endif()

# Upload debug symbols to the Crashpad server

function(upload_debug_symbols TARGET_NAME)
    set(CRASHPAD_UPLOAD_URL
        "https://prod-crashsymbolcollector-pa.googleapis.com"
    )

    get_target_property(REAL_OUTPUT_NAME ${TARGET_NAME} OUTPUT_NAME)
    if(NOT REAL_OUTPUT_NAME)
        set(REAL_OUTPUT_NAME ${TARGET_NAME})
    endif()

    if(WIN32)
        set(DEBUG_FILE "$<TARGET_PDB_FILE:${TARGET_NAME}>")
    elseif(APPLE)
        set(DEBUG_FILE "$<TARGET_FILE:${TARGET_NAME}>.dSYM")
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND dsymutil $<TARGET_FILE:${TARGET_NAME}>
            WORKING_DIRECTORY "${BINARY_OUTPUT_DIR}"
            COMMENT "Generating debug symbols bundle (dSYM)"
            VERBATIM
        )
    elseif(UNIX)
        set(DEBUG_FILE "$<TARGET_FILE:${TARGET_NAME}>")
    endif()

    set(BINARY_OUTPUT_DIR "$<TARGET_FILE_DIR:${TARGET_NAME}>")
    set(SYM_FILENAME "${REAL_OUTPUT_NAME}.sym")

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND $<TARGET_FILE:dump_syms> ${DEBUG_FILE} > "${SYM_FILENAME}"
        WORKING_DIRECTORY "${BINARY_OUTPUT_DIR}"
        COMMENT "Generating debug symbols: ${BINARY_OUTPUT_DIR}/${SYM_FILENAME}"
        VERBATIM
    )

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND
            "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/scripts/upload_debug_symbols_to_crashpad.py"
            "${SYM_FILENAME}" "${CRASHPAD_UPLOAD_URL}"
        WORKING_DIRECTORY "${BINARY_OUTPUT_DIR}"
        COMMENT
            "Uploading debug symbols using upload_debug_symbols_to_crashpad.py..."
        VERBATIM
    )
endfunction()
