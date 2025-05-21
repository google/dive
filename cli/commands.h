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

#include <iosfwd>
#include <map>
#include <string>

#include "cli/cli.h"

namespace Dive
{
namespace cli
{

class Command
{
public:
    enum Visibility
    {
        kHidden = 0,
        kNormal = 1,
        kInternal = 2,
    };
    Command(const char* name, Visibility Visibility);
    virtual ~Command();
    // `at`: command line arg currently being processed
    virtual int         operator()(int argc, int at, char** argv) const = 0;
    virtual int         Help(int argc, int at, char** argv) const = 0;
    virtual std::string Description() const = 0;

    inline Visibility         GetVisibility() const { return m_visibility; }
    inline const std::string& GetName() const { return m_name; }

protected:
    static std::string ProgramName(const char* fullpath);

private:
    std::string m_name;
    Visibility  m_visibility;
};

struct HelpCommand;
struct VersionCommand;
struct ExtractCommand;
struct ModifyGFXRCommand;

// Internal utilities, originally from capture_reporter.
// Hiding from user as they are not intended for normal end user flow.
struct PacketCommand;
struct InfoCommand;
struct RawPM4Command;

template<typename T> struct CommandOf
{
    static const Command& Get();
};

template<> struct CommandOf<HelpCommand>
{
    static const Command& Get(const std::map<std::string, const Command*>*);
};

}  // namespace cli
}  // namespace Dive
