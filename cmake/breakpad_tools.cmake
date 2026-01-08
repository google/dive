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

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(BREAKPAD_ROOT
    "${CMAKE_SOURCE_DIR}/third_party/breakpad"
    CACHE PATH
    "Path to Breakpad source"
)

if(WIN32)
    if(
        EXISTS
            "${BREAKPAD_ROOT}/src/tools/windows/dump_syms/Release/dump_syms.exe"
    )
        add_executable(dump_syms IMPORTED GLOBAL)
        set_property(
            TARGET dump_syms
            PROPERTY
                IMPORTED_LOCATION
                    "${BREAKPAD_ROOT}/src/tools/windows/dump_syms/Release/dump_syms.exe"
        )
    else()
        message(
            WARNING
            "Windows dump_syms build is complex. If this fails, download dump_syms.exe manually."
        )
        add_executable(
            dump_syms
            "${BREAKPAD_ROOT}/src/tools/windows/dump_syms/dump_syms.cc"
            # Windows source list is very long and depends on VS SDKs
        )
        target_include_directories(dump_syms PRIVATE "${BREAKPAD_ROOT}/src")
        target_link_libraries(dump_syms PRIVATE diaguids.lib imagehlp.lib)
    endif()

    add_executable(
        sym_upload
        "${BREAKPAD_ROOT}/src/tools/windows/symupload/symupload.cc"
        "${BREAKPAD_ROOT}/src/common/windows/http_upload.cc"
    )
    target_include_directories(sym_upload PRIVATE "${BREAKPAD_ROOT}/src")
    target_link_libraries(sym_upload PRIVATE wininet.lib)
elseif(APPLE)
    enable_language(OBJCXX)
    enable_language(OBJC)

    add_executable(
        dump_syms
        "${BREAKPAD_ROOT}/src/tools/mac/dump_syms/dump_syms.cc"
        "${BREAKPAD_ROOT}/src/common/mac/dump_syms.cc"
        "${BREAKPAD_ROOT}/src/common/mac/file_id.cc"
        "${BREAKPAD_ROOT}/src/common/mac/macho_id.cc"
        "${BREAKPAD_ROOT}/src/common/mac/macho_reader.cc"
        "${BREAKPAD_ROOT}/src/common/mac/macho_utilities.cc"
        "${BREAKPAD_ROOT}/src/common/mac/macho_walker.cc"
        "${BREAKPAD_ROOT}/src/common/mac/arch_utilities.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/bytereader.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/dwarf2diehandler.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/dwarf2reader.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/elf_reader.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_cfi_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_cu_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_line_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_range_list_handler.cc"
        "${BREAKPAD_ROOT}/src/common/stabs_reader.cc"
        "${BREAKPAD_ROOT}/src/common/stabs_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/module.cc"
        "${BREAKPAD_ROOT}/src/common/language.cc"
        "${BREAKPAD_ROOT}/src/common/path_helper.cc"
        "${BREAKPAD_ROOT}/src/common/md5.cc"
    )
    set_target_properties(dump_syms PROPERTIES CXX_STANDARD 17)
    target_include_directories(dump_syms PRIVATE "${BREAKPAD_ROOT}/src")
    target_link_libraries(
        dump_syms
        "-framework Foundation"
        "-framework CoreFoundation"
    )

    add_executable(
        sym_upload
        "${BREAKPAD_ROOT}/src/tools/mac/symupload/symupload.m"
        "${BREAKPAD_ROOT}/src/common/mac/HTTPMultipartUpload.m"
        "${BREAKPAD_ROOT}/src/common/mac/symbol_collector_client.m"
    )
    target_include_directories(sym_upload PRIVATE "${BREAKPAD_ROOT}/src")
    target_link_libraries(sym_upload "-framework Cocoa" "-framework Foundation")
elseif(UNIX)
    find_package(ZLIB REQUIRED)
    find_package(CURL REQUIRED)

    add_executable(
        dump_syms
        "${BREAKPAD_ROOT}/src/tools/linux/dump_syms/dump_syms.cc"
        "${BREAKPAD_ROOT}/src/common/linux/dump_symbols.cc"
        "${BREAKPAD_ROOT}/src/common/linux/elf_symbols_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/linux/elfutils.cc"
        "${BREAKPAD_ROOT}/src/common/linux/file_id.cc"
        "${BREAKPAD_ROOT}/src/common/linux/memory_mapped_file.cc"
        "${BREAKPAD_ROOT}/src/common/linux/safe_readlink.cc"
        "${BREAKPAD_ROOT}/src/common/linux/linux_libc_support.cc"
        "${BREAKPAD_ROOT}/src/common/linux/crc32.cc"
        "${BREAKPAD_ROOT}/src/common/path_helper.cc"
        "${BREAKPAD_ROOT}/src/common/stabs_reader.cc"
        "${BREAKPAD_ROOT}/src/common/stabs_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/bytereader.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/dwarf2diehandler.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/dwarf2reader.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf/elf_reader.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_cfi_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_cu_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_line_to_module.cc"
        "${BREAKPAD_ROOT}/src/common/dwarf_range_list_handler.cc"
        "${BREAKPAD_ROOT}/src/common/module.cc"
        "${BREAKPAD_ROOT}/src/common/language.cc"
    )
    target_include_directories(
        dump_syms
        PRIVATE "${BREAKPAD_ROOT}/src" ${CMAKE_SOURCE_DIR}
    )
    target_compile_definitions(dump_syms PRIVATE HAVE_A_OUT_H)
    target_link_libraries(dump_syms pthread dl ZLIB::ZLIB)

    add_executable(
        sym_upload
        "${BREAKPAD_ROOT}/src/tools/linux/symupload/sym_upload.cc"
        "${BREAKPAD_ROOT}/src/common/linux/http_upload.cc"
        "${BREAKPAD_ROOT}/src/common/linux/symbol_upload.cc"
        "${BREAKPAD_ROOT}/src/common/linux/libcurl_wrapper.cc"
        "${BREAKPAD_ROOT}/src/common/linux/symbol_collector_client.cc"
        "${BREAKPAD_ROOT}/src/common/path_helper.cc"
    )
    target_include_directories(
        sym_upload
        PRIVATE "${BREAKPAD_ROOT}/src" ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(sym_upload pthread dl CURL::libcurl)
endif()

function(upload_debug_symbols TARGET_NAME)
    set(CRASHPAD_UPLOAD_URL
        "https://prod-crashsymbolcollector-pa.googleapis.com"
    )
    add_dependencies(${TARGET_NAME} dump_syms sym_upload)

    get_target_property(REAL_OUTPUT_NAME ${TARGET_NAME} OUTPUT_NAME)
    if(NOT REAL_OUTPUT_NAME)
        set(REAL_OUTPUT_NAME ${TARGET_NAME})
    endif()

    if(WIN32)
        set(DEBUG_FILE "$<TARGET_PDB_FILE:${TARGET_NAME}>")
    elseif(APPLE)
        set(DEBUG_FILE "$<TARGET_FILE:${TARGET_NAME}>.dSYM")
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

    if(DEFINED ENV{CRASHPAD_API_KEY})
        set(CRASHPAD_API_KEY "$ENV{CRASHPAD_API_KEY}")
    else()
        message(
            FATAL_ERROR
            "CRASHPAD_API_KEY environment variable is not set. Symbol upload will fail."
        )
    endif()

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND
            $<TARGET_FILE:sym_upload> "-p" "sym-upload-v2" "-k"
            "${CRASHPAD_API_KEY}" "${SYM_FILENAME}" "${CRASHPAD_UPLOAD_URL}"
        WORKING_DIRECTORY "${BINARY_OUTPUT_DIR}"
        COMMENT "Uploading debug symbols for ${REAL_OUTPUT_NAME}"
        VERBATIM
    )
endfunction()
