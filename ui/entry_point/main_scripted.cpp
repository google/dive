/*
 Copyright 2025 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "absl/flags/flag.h"
#include "ui/main.h"

ABSL_FLAG(std::string, test_output, "", "Output directory for tests.");
ABSL_FLAG(std::string, test_prefix, "", "Filename prefix for tests.");
ABSL_FLAG(std::string, test_scenario, "", "Execute test scenario.");

int main(int argc, char** argv)
{
    DiveUIMain dive_ui(argc, argv);
    dive_ui.SetOptions({
    .output = absl::GetFlag(FLAGS_test_output),
    .prefix = absl::GetFlag(FLAGS_test_prefix),
    .scenario = absl::GetFlag(FLAGS_test_scenario),
    });
    return dive_ui.Run();
}
