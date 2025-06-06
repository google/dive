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
#include <vector>

#include "commands.h"
#include "pm4_info.h"

using namespace Dive::cli;

int main(int argc, char** argv)
{
    Dive::cli::Init();
    Pm4InfoInit();

    std::map<std::string, const Command*> commands;
    std::vector<const Command*>           commandlist = {
        // public
        &CommandOf<HelpCommand>::Get(&commands),
        &CommandOf<VersionCommand>::Get(),
        &CommandOf<ExtractCommand>::Get(),
        // Internal, use `divecli help --internal`
        // It's hidden to not cause confusion.
        &CommandOf<PacketCommand>::Get(),
        &CommandOf<InfoCommand>::Get(),
        &CommandOf<RawPM4Command>::Get(),
    };
    for (auto cmd : commandlist)
    {
        commands[cmd->GetName()] = cmd;
    }

    if (argc > 1)
    {
        auto iter = commands.find(argv[1]);
        if (iter != commands.end())
        {
            return (*iter->second)(argc, 1, argv);
        }
    }

    return (*commands.find("help")->second)(argc, 0, argv);
}
