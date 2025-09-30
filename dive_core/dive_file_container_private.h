/*
 Copyright 2025 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

namespace Dive
{

struct DiveFileContainerHeader
{
    // Unix Standard Tar compatible format:
    char name[100] = {};             // Filename
    char mode[8] = "0000644";        // Ignored
    char uid[8] = "0000000";         // Ignored
    char gid[8] = "0000000";         // Ignored
    char size[12] = "00000000000";   // File size in octal
    char mtime[12] = "00000000000";  // Ignored
    char checksum[8] = {};           // Ignored
    char filetype = '0';             // Ignored
    char linkname[100] = {};         // Ignored
    char magic[6] = "ustar";         // Ignored
    char version[2] = { '0', '0' };  // Ignored
    char uname[32] = "root";         // Ignored
    char gname[32] = "root";         // Ignored
    char devmajor[8] = {};           // Ignored
    char devminor[8] = {};           // Ignored
    char prefix[155] = {};           // Ignored
    char padding[12] = {};           // Ignored
};

static_assert(sizeof(DiveFileContainerHeader) == 512, "Expect header size to be 512.");

}  // namespace Dive
