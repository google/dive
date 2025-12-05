# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

function(
    DoGenerateThirdPartyLicenseFile
    OUTPUT_FILE
    INPUT_DIRECTORY
    APPEND_FILES
)
    get_filename_component(INPUT_DIRECTORY "${INPUT_DIRECTORY}" ABSOLUTE)
    get_filename_component(OUTPUT_FILE "${OUTPUT_FILE}" ABSOLUTE)

    file(
        GLOB_RECURSE files
        RELATIVE "${INPUT_DIRECTORY}"
        "${INPUT_DIRECTORY}/*"
    )
    file(REMOVE "${OUTPUT_FILE}")

    foreach(file ${files})
        file(
            APPEND
            "${OUTPUT_FILE}"
            "================================================================================\n"
        )
        file(APPEND "${OUTPUT_FILE}" "${file}:\n\n")
        file(READ "${INPUT_DIRECTORY}/${file}" content)
        file(APPEND "${OUTPUT_FILE}" "${content}")
        file(APPEND "${OUTPUT_FILE}" "\n\n\n")
    endforeach()

    if(APPEND_FILES)
        foreach(file ${APPEND_FILES})
            file(READ "${file}" content)
            file(APPEND "${OUTPUT_FILE}" "${content}")
        endforeach()
    endif()
endfunction()

set(GEN_THIRD_PARTY_SCRIPT "${CMAKE_CURRENT_LIST_FILE}")

function(GenerateThirdPartyLicenseFile OUTPUT_FILE INPUT_DIRECTORY)
    cmake_parse_arguments(GEN "" "" "APPEND_FILES" ${ARGN})

    get_filename_component(INPUT_DIRECTORY "${INPUT_DIRECTORY}" ABSOLUTE)
    get_filename_component(OUTPUT_FILE "${OUTPUT_FILE}" ABSOLUTE)
    string(MD5 OUTPUT_FILE_HASH "${OUTPUT_FILE}")
    string(SUBSTRING "${OUTPUT_FILE_HASH}" 0 12 OUTPUT_FILE_HASH)
    add_custom_target(
        "ThirdPartyLicenseFile_${OUTPUT_FILE_HASH}"
        ALL
        COMMAND
            ${CMAKE_COMMAND} -DIN_GENERATE_THIRD_PARTY_LICENSE_FILE=TRUE
            -DINPUT_DIRECTORY="${INPUT_DIRECTORY}"
            -DOUTPUT_FILE="${OUTPUT_FILE}" -DAPPEND_FILES="${GEN_APPEND_FILES}"
            -P "${GEN_THIRD_PARTY_SCRIPT}"
        BYPRODUCTS "${OUTPUT_FILE}"
        COMMENT "Collecting licenses for the ${OUTPUT_FILE} file..."
    )

    if(NOT TARGET "ThirdPartyLicenseFile_all")
        add_custom_target("ThirdPartyLicenseFile_all" ALL)
    endif()
    add_dependencies(
        "ThirdPartyLicenseFile_all"
        "ThirdPartyLicenseFile_${OUTPUT_FILE_HASH}"
    )
endfunction()

if(IN_GENERATE_THIRD_PARTY_LICENSE_FILE)
    DoGenerateThirdPartyLicenseFile(
        "${OUTPUT_FILE}"
        "${INPUT_DIRECTORY}"
        "${APPEND_FILES}"
    )
endif()
