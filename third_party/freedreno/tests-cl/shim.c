/*
 * Copyright (c) 2018 Rob Clark <robdclark@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>

/* Some random shims to link CL programs without dragging too many other
 * android dependencies
 */

int property_get(const char *key, char *value, const char *default_value)
{
	printf("property_get(\"%s\")\n", key);
	return 0;
}

int32_t atrace_is_ready;
//uint64_t atrace_enabled_tags;

void atrace_setup(void)
{
	atrace_is_ready = 1;
//	atrace_enabled_tags = ~0;
}

void atrace_begin_body(const char *name)
{
	printf("atrace_begin_body(\"%s\")\n", name);
}

void atrace_int_body(const char *name, int32_t value)
{
	printf("atrace_int_body(\"%s\", %d)\n", name, value);
}
