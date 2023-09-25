/*
 * Copyright (c) 2012-2018 Rob Clark <robdclark@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

/* old-style program binary XOR'd ascii w/ 0xff */
#ifndef ASCII_XOR
#  define ASCII_XOR 0
#endif

/* convert float to dword */
static inline float d2f(uint32_t d)
{
	union {
		float f;
		uint32_t d;
	} u = {
		.d = d,
	};
	return u.f;
}

static inline void dump_hex(char *buf, int sz)
{
	uint8_t *ptr = (uint8_t *)buf;
	uint8_t *end = ptr + sz;
	int i = 0;

	while (ptr < end) {
		uint32_t d = 0;

		printf((i % 8) ? " " : "\t");

		d |= *(ptr++) <<  0;
		d |= *(ptr++) <<  8;
		d |= *(ptr++) << 16;
		d |= *(ptr++) << 24;

		printf("%08x", d);

		if ((i % 8) == 7) {
			printf("\n");
		}

		i++;
	}

	if (i % 8) {
		printf("\n");
	}
}

static inline void dump_float(char *buf, int sz)
{
	uint8_t *ptr = (uint8_t *)buf;
	uint8_t *end = ptr + sz - 3;
	int i = 0;

	while (ptr < end) {
		uint32_t d = 0;

		printf((i % 8) ? " " : "\t");

		d |= *(ptr++) <<  0;
		d |= *(ptr++) <<  8;
		d |= *(ptr++) << 16;
		d |= *(ptr++) << 24;

		printf("%8f", d2f(d));

		if ((i % 8) == 7) {
			printf("\n");
		}

		i++;
	}

	if (i % 8) {
		printf("\n");
	}
}

#define is_ok_ascii(c) \
	(isascii(c) && ((c == '\t') || !iscntrl(c)))

static void clean_ascii(char *buf, int sz)
{
	uint8_t *ptr = (uint8_t *)buf;
	uint8_t *end = ptr + sz;
	while (ptr < end) {
		*(ptr++) ^= ASCII_XOR;
	}
}

static void dump_ascii(char *buf, int sz)
{
	uint8_t *ptr = (uint8_t *)buf;
	uint8_t *end = ptr + sz;
	printf("\t");
	while (ptr < end) {
		uint8_t c = *(ptr++) ^ ASCII_XOR;
		if (c == '\n') {
			printf("\n\t");
		} else if (c == '\0') {
			printf("\n\t-----------------------------------\n\t");
		} else if (is_ok_ascii(c)) {
			printf("%c", c);
		} else {
			printf("?");
		}
	}
	printf("\n");
}

static void dump_hex_ascii(char *buf, int sz)
{
	uint8_t *ptr = (uint8_t *)buf;
	uint8_t *end = ptr + sz;
	uint8_t *ascii = ptr;
	int i = 0;

	printf("-----------------------------------------------\n");
	printf("%d (0x%x) bytes\n", sz, sz);

	while (ptr < end) {
		uint32_t d = 0;

		printf((i % 4) ? " " : "\t");

		d |= *(ptr++) <<  0;
		d |= *(ptr++) <<  8;
		d |= *(ptr++) << 16;
		d |= *(ptr++) << 24;

		printf("%08x", d);

		if ((i % 4) == 3) {
			int j;
			printf("\t|");
			for (j = 0; j < 16; j++) {
				uint8_t c = *(ascii++);
				c ^= ASCII_XOR;
				printf("%c", (isascii(c) && !iscntrl(c)) ? c : '.');
			}
			printf("|\n");
		}

		i++;
	}

	if (i % 8) {
		int j;
		printf("\t|");
		while (ascii < end) {
			uint8_t c = *(ascii++);
			c ^= ASCII_XOR;
			printf("%c", (isascii(c) && !iscntrl(c)) ? c : '.');
		}
		printf("|\n");
	}
}

static inline const char *tab(int lvl)
{
	const char *TAB = "\t\t\t\t\t";
	return &TAB[strlen(TAB) - lvl];
}

#endif /* __UTIL_H__ */
