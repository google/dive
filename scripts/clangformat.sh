#!/bin/bash

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

CLANG_FORMAT=./clang_format/clang-format-18

$CLANG_FORMAT --version

function check() {
  local name=$1; shift
  echo -n "Running check $name... "

  if ! "$@"; then
    echo "${red}FAILED${normal}"
    echo "  Error executing: $@";
    exit 1
  fi

  if ! git diff --quiet HEAD; then
    echo "${red}FAILED${normal}"
    echo "  Git workspace not clean:"
    git --no-pager diff -p HEAD
    echo "${red}Check $name failed.${normal}"
    exit 1
  fi

  echo "${green}OK${normal}"
}

function run_copyright_headers() {
  tmpfile=`mktemp`
  for suffix in "cc" "cpp" "frag" "glsl" "go" "h" "hpp" "java" "js" "sh" "vert" "xml"; do
    # Grep flag '-L' print files that DO NOT match the copyright regex
    # Filter out third party files
    # Filter out anything in driver/ folder
    # Filter out anything in dive_core/llvm_libs/ folder
    # Grep seems to match "(standard input)", filter this out in the for loop output
    git diff origin/main HEAD --name-only --| grep "\.${suffix}$" | grep -v "^third_party/" | grep -P -v '^prebuild/*' | xargs grep -L "Copyright .* Google"
  done | grep -v "(standard input)" > ${tmpfile}
  if test -s ${tmpfile}; then
    # tempfile is NOT empty
    echo "Copyright issue in these files:"
    cat ${tmpfile}
    rm ${tmpfile}
    return 1
  else
    rm ${tmpfile}
    return 0
  fi
}


function run_clang_format() {
  # Use grep negative lookahead to filter out all files that being with 'driver' but does not contain 'dive'
  git diff origin/main HEAD --name-only --diff-filter=ACMR --| grep -E "\.(c|cpp|h|hpp|cc)$" | grep -P -v '^third_party/*' | grep -P -v '^prebuild/*' | xargs $CLANG_FORMAT -i -style=file
}

# Check copyright headers
check copyright-headers run_copyright_headers

# Check clang-format.
check clang-format run_clang_format
