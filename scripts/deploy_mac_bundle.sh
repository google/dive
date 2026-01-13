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

PROJECT_ROOT="$(readlink -f $0)"
PROJECT_ROOT="${PROJECT_ROOT%/*}/.."
readonly PROJECT_ROOT="$(readlink -f ${PROJECT_ROOT})"
INSTALL_DIR="pkg"
SIGN_BUNDLE=true
readonly START_TIME=`date +%r`

if [ $# -ne 0 ]; then
    if [ $# -ne 1 ]; then
        echo "Too many arguments"
        echo "Valid usage: 'deploy_mac_bundle.sh [--no-sign]'"
        exit 1
    fi
    if [ "$1" = "--no-sign" ]; then
        SIGN_BUNDLE=false
    else
        echo "Invalid parameter: $1"
        echo "Valid usage: 'deploy_mac_bundle.sh [--no-sign]'"
        exit 1
    fi
fi

echo "Install dir: ${INSTALL_DIR}"

pushd ${PROJECT_ROOT}

echo "current dir " `pwd`

if [ ! -d ${INSTALL_DIR}/dive.app ]; then
    echo .
    echo "Moving over dive.app and running macdeployqt"

    mv ${INSTALL_DIR}/host/dive.app ${INSTALL_DIR} || exit 1
    macdeployqt ${INSTALL_DIR}/dive.app || exit 1
else
    echo .
    echo "Skipping moving dive.app and running macdeployqt"
fi

echo "Copying resources into app bundle"
cp -r ${INSTALL_DIR}/device/* ${INSTALL_DIR}/dive.app/Contents/Resources/ || exit 1
cp -r ${INSTALL_DIR}/host/* ${INSTALL_DIR}/dive.app/Contents/MacOS/ || exit 1
mkdir -p ${INSTALL_DIR}/dive.app/Contents/Resources/plugins/ || exit 1
cp -r ${INSTALL_DIR}/plugins/* ${INSTALL_DIR}/dive.app/Contents/Resources/plugins/ || exit 1

if ${SIGN_BUNDLE}; then
    echo .
    echo "Ad-hoc signing the application bundle"
    codesign --force --deep --sign - ${INSTALL_DIR}/dive.app || exit 1
else
    echo .
    echo "Skipping signing step"
fi

popd

echo .
echo "Start Time:" ${START_TIME}
echo "Finish Time:" `date +%r`
