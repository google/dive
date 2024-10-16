/*
 Copyright 2024 Google LLC

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

#include "google_dive_ext.h"

char *strndup(const char *s, size_t n)
{
    size_t len = strnlen(s, n);
    char  *new_str = (char *)malloc(len + 1);
    if (new_str == NULL)
    {
        return NULL;
    }
    memcpy(new_str, s, len);
    new_str[len] = '\0';
    return new_str;
}

int vasprintf(char **strp, const char *fmt, va_list ap)
{
    va_list ap_copy;
    va_copy(ap_copy, ap);

    int len = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);

    if (len < 0)
    {
        return -1;
    }

    *strp = (char *)malloc(len + 1);
    if (!*strp)
    {
        return -1;
    }

    return vsnprintf(*strp, len + 1, fmt, ap);
}

int futex_wait(uint32_t *addr, int32_t value, const struct timespec *timeout)
{
   return WaitForSingleObject(addr, value);
}

int futex_wake(uint32_t *addr, int count)
{
    return SetEvent(addr);
}
