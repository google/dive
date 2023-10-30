
/*
 Copyright 2020 Google LLC

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

#include "common.h"

#include <stdarg.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

void DIVE_LOG_INTERNAL(const char *file, int line, const char *format, ...)
{
    va_list args;
    char    str[8 * 1024];
    va_start(args, format);
    vsnprintf(str, sizeof(str), format, args);
    va_end(args);

    fprintf(stdout, "%s(%d): %s", file, line, str);
    fflush(stdout);
}
