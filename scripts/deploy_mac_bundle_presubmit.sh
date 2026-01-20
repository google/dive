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

# This script is used by Github workflows to create the app bundle on macOS runners, since the presubmit folder structure is different from dev builds.

set -euo pipefail          
          
mv ./pkg/host/dive.app ./pkg/
macdeployqt ./pkg/dive.app
cp -r ./pkg/host/* ./pkg/dive.app/Contents/MacOS/
mkdir -p ./pkg/dive.app/Contents/Resources/plugins/