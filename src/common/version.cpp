/*
Copyright 2025 Google Inc.

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

#include "dive/common/version.h"

#include <ostream>

#include "dive/common/version_defs.h"

namespace Dive
{
std::ostream& operator<<(std::ostream& stream, const DiveVersion& version)
{
    stream << version.major << "." << version.minor << "." << version.rev;
    if (!version.commit.empty())
    {
        auto short_commit = std::string(version.commit);
        if (short_commit.size() > 8)
        {
            short_commit.resize(8);
        }
        stream << " (" << version.commit << ")";
    }
    return stream;
}

void DiveVersion::PrintLongVersion(std::ostream& stream) const {}

DiveVersion DiveVersion::Get()
{
    return {
        DIVE_VERSION_MAJOR,
        DIVE_VERSION_MINOR,
        DIVE_VERSION_REVISION,
        DIVE_VERSION_SHA1,
    };
};

DiveApplicationInfo DiveApplicationInfo::Get()
{
    return {
        DIVE_PRODUCT_NAME,
        DIVE_PRODUCT_DESCRIPTION,
        DIVE_COPYRIGHT_DESCRIPTION,
        DiveVersion::Get(),
    };
}

std::ostream& operator<<(std::ostream& stream, const DiveApplicationInfo& info)
{
    stream << info.name << std::endl;
    stream << info.description << std::endl;
    stream << info.copyright << std::endl;
    stream << std::endl;
    stream << "Version " << info.version;
    return stream;
}

}  // namespace Dive
