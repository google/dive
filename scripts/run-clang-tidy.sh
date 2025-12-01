#!/bin/bash

# Copyright 2025 Google LLC
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

set -ex

readonly DIVE_ROOT="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )"
readonly GIT_DIFF_BASE="${GIT_DIFF_BASE:="origin/main"}"

if [[ ! -d "${DIVE_ROOT}/build" ]]; then
    echo "Build directory ${DIVE_ROOT}/build not found"
    exit
fi

if ! which clang-tidy ; then
  echo "clang-tidy not found"
  exit
fi

pushd "${DIVE_ROOT}"
  git diff "${GIT_DIFF_BASE}" --name-only --diff-filter=ACMR -- \
    | grep -E "\.(c|cpp|h|hpp|cc)$" | grep -E -v '^third_party/*' \
    | grep -E -v '^prebuild/*' \
    | xargs -r clang-tidy -p build
popd
