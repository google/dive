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

set -ex

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PREBUILT_DIR="${SCRIPT_DIR}/prebuilt/mac"

if [[ ! -e $1 ]]; then
  echo "Commit hash file not found: $1"
  exit 1
fi
readonly COMMIT_HASH_FILE="$1"
readonly COMMIT_HASH="$(cat <"${COMMIT_HASH_FILE}")"
readonly DEPOT_TOOLS_URL="https://chromium.googlesource.com/chromium/tools/depot_tools"

if [[ ! -e depot_tools ]]; then
  git clone "${DEPOT_TOOLS_URL}" depot_tools
fi
export PATH="$(pwd)/depot_tools:${PATH}"

if [[ ! -e crashpad ]]; then
  fetch crashpad
fi

pushd crashpad
git fetch origin "${COMMIT_HASH}"
git checkout "${COMMIT_HASH}"
gclient sync

gn gen out/Default --args="is_debug=false"
ninja -C out/Default

echo "Extracting libutil.a..."
mkdir -p out/Default/obj/util/libutil_extracted
cd out/Default/obj/util/libutil_extracted
ar -x ../libutil.a
cd -

echo "Merging objects..."

find out/Default/obj/util/libutil_extracted out/Default/obj/util/mach \
    -name "*.o" \
    ! -name "*_test.o" \
    > valid_objects.txt

libtool -static -filelist valid_objects.txt -o out/Default/libutil_merged.a

echo "Caching artifacts to ${PREBUILT_DIR}..."
mkdir -p "${PREBUILT_DIR}"

cp -f out/Default/crashpad_handler "${PREBUILT_DIR}/"
cp -f out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libcommon.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libclient.a "${PREBUILT_DIR}/"
cp -f out/Default/libutil_merged.a "${PREBUILT_DIR}/libutil.a"

popd
