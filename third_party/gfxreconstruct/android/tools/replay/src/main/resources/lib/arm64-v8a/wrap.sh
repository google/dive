#!/system/bin/sh
#
# Copyright 2025 Google LLC
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

capture_pm4_enabled=$(getprop debug.dive.replay.capture_pm4)

echo "capture_pm4_enabled is " $capture_pm4_enabled
if [[ "$capture_pm4_enabled" == "true" || "$capture_pm4_enabled" == "1" ]]; then
    export LD_PRELOAD=/data/local/tmp/libwrap.so 
fi

"$@"
