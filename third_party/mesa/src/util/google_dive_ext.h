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

#ifndef _GOOGLE_DIVE_EXT_H
#define _GOOGLE_DIVE_EXT_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * The following functions are not directly 
 * available in Windows, so these implementations
 * are used when compiling with MSVC.
 */
char *strndup(const char *s, size_t n);
int vasprintf(char **strp, const char *fmt, va_list ap);
int futex_wait(uint32_t *addr, int32_t value, const struct timespec *timeout);
int futex_wake(uint32_t *addr, int count);


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _GOOGLE_DIVE_EXT_H*/
