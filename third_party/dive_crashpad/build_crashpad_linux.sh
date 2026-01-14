#!/bin/bash
set -ex

# 1. Setup paths
# Get the directory where this script resides to find 'prebuilt/'
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PREBUILT_DIR="${SCRIPT_DIR}/prebuilt/linux"

if [[ ! -e $1 ]]; then
  echo "Commit hash file not found: $1"
  exit 1
fi
readonly COMMIT_HASH_FILE="$1"
readonly COMMIT_HASH="$(cat <"${COMMIT_HASH_FILE}")"
readonly DEPOT_TOOLS_URL="https://chromium.googlesource.com/chromium/tools/depot_tools"

# 2. Setup Depot Tools
if [[ ! -e depot_tools ]]; then
  git clone "${DEPOT_TOOLS_URL}" depot_tools
fi
export PATH="$(pwd)/depot_tools:${PATH}"

# 3. Fetch
if [[ ! -e crashpad ]]; then
  fetch crashpad
fi

# 4. Build
pushd crashpad
git fetch origin "${COMMIT_HASH}"
git checkout "${COMMIT_HASH}"
gclient sync

gn gen out/Default --args="is_debug=false target_cpu=\"x64\" extra_cflags=\"-fPIC\""
ninja -C out/Default

# 5. Cache Artifacts (Copy back to source tree)
echo "Caching artifacts to ${PREBUILT_DIR}..."
mkdir -p "${PREBUILT_DIR}"

cp -f out/Default/crashpad_handler "${PREBUILT_DIR}/"
cp -f out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libcommon.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libclient.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/util/libutil.a "${PREBUILT_DIR}/"

popd
