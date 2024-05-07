/*
Copyright 2023 Google Inc.

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

#include "layer_common.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "capture_service/log.h"
#include "capture_service/server.h"

namespace DiveLayer
{

bool IsLibwrapLoaded()
{
    bool loaded = false;
#if defined(__ANDROID__)
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps)
    {
        return loaded;
    }

    char  *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, maps) > 0)
    {
        if (strstr(line, "libwrap.so"))
        {
            loaded = true;
            break;
        }
    }
    free(line);
    fclose(maps);
#endif
    return loaded;
}

ServerRunner::ServerRunner()
{
    is_libwrap_loaded = IsLibwrapLoaded();
    LOGI("libwrap loaded: %d", is_libwrap_loaded);
    if (is_libwrap_loaded)
    {
        server_thread = std::thread(Dive::server_main);
    }
}

ServerRunner::~ServerRunner()
{
    if (is_libwrap_loaded && server_thread.joinable())
    {
        LOGI("Wait for server thread to join");
        server_thread.join();
    }
}

ServerRunner &GetServerRunner()
{
    static ServerRunner runner;
    return runner;
}

}  // namespace DiveLayer