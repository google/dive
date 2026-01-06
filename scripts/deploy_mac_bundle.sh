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

# This script automates the deployment of the Dive app bundle on macOS. For comprehensive documentation and advanced options, please refer to BUILD.md, "App bundle (macOS)" section

PROJECT_ROOT="$(readlink -f $0)"
PROJECT_ROOT="${PROJECT_ROOT%/*}/.."
readonly PROJECT_ROOT="$(readlink -f ${PROJECT_ROOT})"
INSTALL_DIR="pkg"
readonly START_TIME=`date +%r`

if [ $# -ne 0 ]; then
    INSTALL_DIR=$1
fi
echo "Install dir: ${INSTALL_DIR}"

pushd ${PROJECT_ROOT}

echo "current dir " `pwd`

mv ${INSTALL_DIR}/host/dive.app ${INSTALL_DIR} || exit 1
macdeployqt ${INSTALL_DIR}/dive.app || exit 1

echo "Copying host tools and device resources into app bundle"
cp -r ${INSTALL_DIR}/device/* ${INSTALL_DIR}/dive.app/Contents/Resources/ || exit 1
cp -r ${INSTALL_DIR}/host/* ${INSTALL_DIR}/dive.app/Contents/MacOS/ || exit 1

for dir in ${PROJECT_ROOT}/${INSTALL_DIR}/*/
do
    BASENAME="$(basename -- $dir)"
    if [[ ${BASENAME} == *"plugin" ]]; then
        echo "Copying plugin dir: ${BASENAME}"
        mkdir -p ${INSTALL_DIR}/dive.app/Contents/Resources/${BASENAME}/ || exit 1
        cp -r ${INSTALL_DIR}/${BASENAME}/* ${INSTALL_DIR}/dive.app/Contents/Resources/${BASENAME}/ || exit 1
    else
        echo "Skipping non-plugin dir: ${BASENAME}" 
    fi
done

# Ad-hoc signing the application bundle
codesign --force --deep --sign - ${INSTALL_DIR}/dive.app || exit 1

popd

echo "Start Time:" ${START_TIME}
echo "Finish Time:" `date +%r`
