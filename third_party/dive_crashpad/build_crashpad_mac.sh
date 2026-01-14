#!/bin/bash
set -ex

# 1. Setup paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PREBUILT_DIR="${SCRIPT_DIR}/prebuilt/mac"

if [[ ! -e $1 ]]; then exit 1; fi
readonly COMMIT_HASH="$(cat <"$1")"
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

# Build (auto-detects host cpu)
gn gen out/Default --args="is_debug=false"
ninja -C out/Default

# 5. Cache Artifacts
echo "Caching artifacts to ${PREBUILT_DIR}..."
mkdir -p "${PREBUILT_DIR}"

cp -f out/Default/crashpad_handler "${PREBUILT_DIR}/"
cp -f out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libcommon.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/client/libclient.a "${PREBUILT_DIR}/"
cp -f out/Default/obj/util/libutil.a "${PREBUILT_DIR}/"

popd
