#!/bin/bash

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

validate_and_check_cache() {

    if [ "$#" -ne 7 ]; then
        echo "Error: Unexpected number of arguments."
        echo "Usage: validate_and_check_cache <commit_hash_file> <prebuilt_dir> <metadata_file> <config> <compiler_stamp> <cached_libs> <cached_handler>"
        exit 1
    fi

    readonly COMMIT_HASH_FILE="$1"
    export COMMIT_HASH="$(cat <"${COMMIT_HASH_FILE}")"
    export PREBUILT_DIR="$2"
    export METADATA_FILE="$3"
    export CONFIG="$4"
    readonly COMPILER_STAMP="$5"
    export CURRENT_STAMP="${CONFIG}_${COMPILER_STAMP}"
    readonly CACHED_LIBS_STR="$6"
    readonly CACHED_HANDLER="$7"

    if [[ -f "${METADATA_FILE}" ]]; then
        CACHED_STAMP=$(cat "${METADATA_FILE}" | xargs)

        if [[ "${CACHED_STAMP}" == "${CURRENT_STAMP}" ]]; then
            ALL_FILES_EXIST=true

            if [[ ! -f "${CACHED_HANDLER}" ]]; then
                echo "Crashpad handler missing: ${CACHED_HANDLER}"
                ALL_FILES_EXIST=false
            fi

            if [[ "$ALL_FILES_EXIST" == "true" ]]; then
                IFS=' ' read -ra LIBS_ARRAY <<< "${CACHED_LIBS_STR}"
                for lib in "${LIBS_ARRAY[@]}"; do
                    lib=$(echo "$lib" | xargs)
                    if [[ ! -f "$lib" ]]; then
                        echo "Crashpad library missing: ${lib}"
                        ALL_FILES_EXIST=false
                        break
                    fi
                done
            fi

            if [[ "$ALL_FILES_EXIST" == "true" ]]; then
                echo "All files exist and metadata matches (${CURRENT_STAMP}). Skipping Crashpad build."
                exit 0
            fi
        else
            echo "Crashpad metadata mismatch."
            echo "  Cached:  ${CACHED_STAMP}"
            echo "  Current: ${CURRENT_STAMP}"
        fi
    else
        echo "Crashpad metadata file not found."
    fi
}
