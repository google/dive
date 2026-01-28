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

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
source "${SCRIPT_DIR}/crashpad_common.sh"

validate_and_check_cache "$1" "$2" "$3" "$4" "$5" "$6" "$7"

echo "Starting Crashpad build (Mac_${CONFIG})..."

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

ARCH=$(uname -m)
if [[ "$ARCH" == "arm64" ]]; then
  TARGET_CPU="arm64"
else
  TARGET_CPU="x64"
fi

gn gen out/Default --args="is_debug=false target_cpu=\"${TARGET_CPU}\""
# Build with limited parallelism to prevent CI OOM crashes
ninja -C out/Default -j 4

# We search the entire out/Default directory for files matching "mig_output*.o"
# or the standard "*Server.o"/"*User.o" patterns to be safe.
MIG_OBJS=$(find out/Default -type f \
    \( -name "mig_output*.o" -o -name "*child_portServer.o" -o -name "*child_portUser.o" -o -name "*notifyServer.o" -o -name "*notifyUser.o" \) \
    ! -name "*test*" ! -name "*gtest*")

# Ensure we actually found the critical child_portServer file
if [[ -z "$MIG_OBJS" ]] || [[ "$MIG_OBJS" != *"child_portServer"* ]]; then
  echo "Error: Could not locate critical MIG files (child_portServer) in out/Default"
  echo "Search performed in: $(pwd)/out/Default"
  echo "Found files (if any):"
  echo "$MIG_OBJS"
  exit 1
fi

# Merge original libutil.a + found MIG objects into a new complete archive
/usr/bin/libtool -static -o out/Default/libutil_complete.a \
    out/Default/obj/util/libutil.a \
    $MIG_OBJS

echo "Caching artifacts to ${PREBUILT_DIR}..."
mkdir -p "${PREBUILT_DIR}"

cp -f out/Default/crashpad_handler "${PREBUILT_DIR}/"
cp -f out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libcommon.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libclient.a "${PREBUILT_DIR}/"
cp -f out/Default/libutil_complete.a "${PREBUILT_DIR}/libutil.a"

popd

echo "${CURRENT_STAMP}" > "${METADATA_FILE}"
echo "Crashpad build complete and metadata updated."
