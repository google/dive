/*
 * Copyright (c) 2018 Rob Clark <robdclark@gmail.com>
 * Copyright (c) 2011-2017, The Linux Foundation. All rights reserved.
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

#include <dlfcn.h>

/*
 * Uses some of the APIs exported from libadreno_util.so for use by gralloc
 * (and perhaps others) to help figure out texture layout.  Some parts
 * copy/pasta from libgralloc1/gr_adreno_info.[ch]
 */

// Adreno Pixel Formats
typedef enum {
	ADRENO_PIXELFORMAT_UNKNOWN = 0,
	ADRENO_PIXELFORMAT_R10G10B10A2_UNORM = 24,  // Vertex, Normalized GL_UNSIGNED_INT_10_10_10_2_OES
	ADRENO_PIXELFORMAT_R8G8B8A8 = 28,
	ADRENO_PIXELFORMAT_R8G8B8A8_SRGB = 29,
	ADRENO_PIXELFORMAT_B5G6R5 = 85,
	ADRENO_PIXELFORMAT_B5G5R5A1 = 86,
	ADRENO_PIXELFORMAT_B8G8R8A8 = 90,
	ADRENO_PIXELFORMAT_B8G8R8A8_SRGB = 91,
	ADRENO_PIXELFORMAT_B8G8R8X8_SRGB = 93,
	ADRENO_PIXELFORMAT_NV12 = 103,
	ADRENO_PIXELFORMAT_YUY2 = 107,
	ADRENO_PIXELFORMAT_B4G4R4A4 = 115,
	ADRENO_PIXELFORMAT_NV12_EXT = 506,       // NV12 with non-std alignment and offsets
	ADRENO_PIXELFORMAT_R8G8B8X8 = 507,       //  GL_RGB8 (Internal)
	ADRENO_PIXELFORMAT_R8G8B8 = 508,         //  GL_RGB8
	ADRENO_PIXELFORMAT_A1B5G5R5 = 519,       //  GL_RGB5_A1
	ADRENO_PIXELFORMAT_R8G8B8X8_SRGB = 520,  //  GL_SRGB8
	ADRENO_PIXELFORMAT_R8G8B8_SRGB = 521,    //  GL_SRGB8
	ADRENO_PIXELFORMAT_A2B10G10R10_UNORM = 532,
	// Vertex, Normalized GL_UNSIGNED_INT_10_10_10_2_OES
	ADRENO_PIXELFORMAT_R10G10B10X2_UNORM = 537,
	// Vertex, Normalized GL_UNSIGNED_INT_10_10_10_2_OES
	ADRENO_PIXELFORMAT_R5G6B5 = 610,         //  RGBA version of B5G6R5
	ADRENO_PIXELFORMAT_R5G5B5A1 = 611,       //  RGBA version of B5G5R5A1
	ADRENO_PIXELFORMAT_R4G4B4A4 = 612,       //  RGBA version of B4G4R4A4
	ADRENO_PIXELFORMAT_UYVY = 614,           //  YUV 4:2:2 packed progressive (1 plane)
	ADRENO_PIXELFORMAT_NV21 = 619,
	ADRENO_PIXELFORMAT_Y8U8V8A8 = 620,  // YUV 4:4:4 packed (1 plane)
	ADRENO_PIXELFORMAT_Y8 = 625,        //  Single 8-bit luma only channel YUV format
	ADRENO_PIXELFORMAT_TP10 = 648,
} ADRENOPIXELFORMAT;

// link(s)to adreno surface padding library.
static int (*adreno_compute_padding)(int width, int bpp, int surface_tile_height,
		int screen_tile_height, int padding_threshold);
static void (*adreno_compute_aligned_width_and_height)(int width, int height, int bpp,
		int tile_mode, int raster_mode,
		int padding_threshold, int *aligned_w,
		int *aligned_h);
static void (*adreno_compute_compressedfmt_aligned_width_and_height)(
		int width, int height, int format, int tile_mode, int raster_mode, int padding_threshold,
		int *aligned_w, int *aligned_h, int *bpp);
static int (*adreno_isUBWCSupportedByGpu)(ADRENOPIXELFORMAT format);
static unsigned int (*adreno_get_gpu_pixel_alignment)(void);

static void setup(void)
{
	void *libadreno_utils = dlopen("libadreno_utils.so", RTLD_NOW);
	adreno_compute_aligned_width_and_height =
			dlsym(libadreno_utils, "compute_aligned_width_and_height");
	adreno_compute_padding =
			dlsym(libadreno_utils, "compute_surface_padding");
	adreno_compute_compressedfmt_aligned_width_and_height =
			dlsym(libadreno_utils, "compute_compressedfmt_aligned_width_and_height");
	adreno_isUBWCSupportedByGpu =
			dlsym(libadreno_utils, "isUBWCSupportedByGpu");
	adreno_get_gpu_pixel_alignment =
			dlsym(libadreno_utils, "get_gpu_pixel_alignment");
}

static void dump_aligned_size(int cpp, int tile_mode, int w, int h)
{
	/* some *really* informative comments there from gralloc code: */
	int raster_mode = 0;          // Adreno unknown raster mode.
	int padding_threshold = 512;  // Threshold for padding surfaces.
	int aligned_w, aligned_h;

	adreno_compute_aligned_width_and_height(w, h, cpp, tile_mode, raster_mode,
			padding_threshold, &aligned_w, &aligned_h);

	printf("%dx%d@%d, tile_mode=%d: %dx%d\n", w, h, cpp, tile_mode, aligned_w, aligned_h);
}

int main(int argc, char **argv)
{
	setup();

	dump_aligned_size(2, 1, 100, 100);
}
