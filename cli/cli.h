/*
 Copyright 2021 Google LLC

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

#pragma once

#include <cstdint>
#include <string>

namespace Dive
{
namespace cli
{

//--------------------------------------------------------------------------------------------------
// CLI utilities
void Init();
bool IsConsoleOutput();

const char *RepositoryVersion();
std::string FileFormatVersion();

//--------------------------------------------------------------------------------------------------
// Dive Capture / Crash Analysis related
int ExtractCapture(const char *filename, const char *extract_assets);

//--------------------------------------------------------------------------------------------------
// GFXR Capture related
int ModifyGFXRCapture(const char *original_filename, const char *new_filename);

}  // namespace cli
}  // namespace Dive
