#!/bin/bash

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

# This script automates the deployment of the Dive app bundle on macOS.
# When --presubmit is specified, the following changes will happen:
# - Assume only host tools and sample plugin were built and avoid copying device resources
# - Do not try to switch directory to the PROJECT_ROOT
# - Do not sign the application bundle (same as --no-sign)

set -euo pipefail
readonly INSTALL_DIR="build/pkg"
SIGN_BUNDLE=true
IS_PRESUBMIT=false
readonly START_TIME="$(date +%r)"

if [ $# -ne 0 ]; then
    if [ $# -ne 1 ]; then
        echo "Too many arguments"
        echo "Valid usage: 'deploy_mac_bundle.sh [--presubmit | --no-sign]'"
        exit 1
    fi
    if [ "$1" = "--no-sign" ]; then
        SIGN_BUNDLE=false
    elif [ "$1" = "--presubmit" ]; then
        IS_PRESUBMIT=true
        SIGN_BUNDLE=false
    else
        echo "Invalid parameter: $1"
        echo "Valid usage: 'deploy_mac_bundle.sh [--presubmit | --no-sign]'"
        exit 1
    fi
fi

if [ ! ${IS_PRESUBMIT} ]; then
    readonly PROJECT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." >/dev/null 2>&1 && pwd )"
    pushd ${PROJECT_ROOT}
fi

echo "Current dir: $(pwd)"
echo "Install dir: ${INSTALL_DIR}"

if [ ! -d ${INSTALL_DIR}/dive.app ]; then
    echo .
    echo "Moving over dive.app and running macdeployqt"

    mv ${INSTALL_DIR}/host/dive.app ${INSTALL_DIR}
    macdeployqt ${INSTALL_DIR}/dive.app
else
    echo .
    echo "Skipping moving dive.app and running macdeployqt"
fi

echo "Copying resources into app bundle"
if [ ! ${IS_PRESUBMIT} ]; then
    cp -r ${INSTALL_DIR}/device/* ${INSTALL_DIR}/dive.app/Contents/Resources/
else
    echo "Skipping copying of device resources"
fi
cp -r ${INSTALL_DIR}/host/* ${INSTALL_DIR}/dive.app/Contents/MacOS/
mkdir -p ${INSTALL_DIR}/dive.app/Contents/Resources/plugins/
cp -r ${INSTALL_DIR}/plugins/* ${INSTALL_DIR}/dive.app/Contents/Resources/plugins/

if ${SIGN_BUNDLE}; then
    echo .
    echo "Ad-hoc signing the application bundle"
    codesign --force --deep --sign - ${INSTALL_DIR}/dive.app
else
    echo .
    echo "Skipping signing step"
fi

if [ ! ${IS_PRESUBMIT} ]; then
    popd
fi

echo .
echo "Start Time: ${START_TIME}"
echo "Finish Time: $(date +%r)"
