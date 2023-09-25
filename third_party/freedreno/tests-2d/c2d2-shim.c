/*
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
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

#include <c2d2.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PROLOG(func) 					\
	static typeof(func) *orig_##func = NULL;	\
	if (!orig_##func)				\
		orig_##func = __dlsym_helper(#func);	\

static void * __dlsym_helper(const char *name)
{
	static void *libc2d2_dl;
	void *func;

	if (!libc2d2_dl)
		libc2d2_dl = dlopen("libC2D2.so", RTLD_LAZY);

	if (!libc2d2_dl) {
		printf("Failed to dlopen c2d2: %s\n", dlerror());
		exit(-1);
	}

	func = dlsym(libc2d2_dl, name);

	if (!func) {
		printf("Failed to find %s: %s\n", name, dlerror());
		exit(-1);
	}

	return func;
}


C2D_STATUS c2dCreateSurface(uint32 *surface_id,
		uint32 surface_bits,
		C2D_SURFACE_TYPE surface_type,
		void *surface_definition)
{
	PROLOG(c2dCreateSurface);
	return orig_c2dCreateSurface(surface_id, surface_bits,
			surface_type, surface_definition);
}

C2D_STATUS c2dUpdateSurface(uint32 surface_id,
		uint32 surface_bits,
		C2D_SURFACE_TYPE surface_type,
		void *surface_definition)
{
	PROLOG(c2dUpdateSurface);
	return orig_c2dUpdateSurface(surface_id, surface_bits,
			surface_type, surface_definition);
}

C2D_STATUS c2dReadSurface(uint32 surface_id,
		C2D_SURFACE_TYPE surface_type,
		void *surface_definition,
		int32 x, int32 y)
{
	PROLOG(c2dReadSurface);
	return orig_c2dReadSurface(surface_id, surface_type,
			surface_definition, x, y);
}

C2D_STATUS c2dFillSurface(uint32 surface_id,
		uint32 fill_color,
		C2D_RECT *fill_rect)
{
	PROLOG(c2dFillSurface);
	return orig_c2dFillSurface(surface_id, fill_color, fill_rect);
}

C2D_STATUS c2dDraw(uint32 target_id,
		uint32 target_config, C2D_RECT *target_scissor,
		uint32 target_mask_id, uint32 target_color_key,
		C2D_OBJECT *objects_list, uint32 num_objects)
{
	PROLOG(c2dDraw);
	return orig_c2dDraw(target_id, target_config, target_scissor,
			target_mask_id, target_color_key,
			objects_list, num_objects);

}

C2D_STATUS c2dFinish(uint32 target_id)
{
	PROLOG(c2dFinish);
	return orig_c2dFinish(target_id);

}

C2D_STATUS c2dFlush(uint32 target_id, c2d_ts_handle *timestamp)
{
	PROLOG(c2dFlush);
	return orig_c2dFlush(target_id, timestamp);

}

C2D_STATUS c2dWaitTimestamp(c2d_ts_handle timestamp)
{
	PROLOG(c2dWaitTimestamp);
	return orig_c2dWaitTimestamp(timestamp);
}

C2D_STATUS c2dDestroySurface(uint32 surface_id)
{
	PROLOG(c2dDestroySurface);
	return orig_c2dDestroySurface(surface_id);
}

