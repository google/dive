/*
 * Copyright (c) 2017 Rob Clark <robdclark@gmail.com>
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

#include <GLES3/gl32.h>
#include "test-util-3d.h"

static EGLint const config_attribute_list[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_DEPTH_SIZE, 8,
	EGL_NONE
};

static const EGLint context_attribute_list[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

static EGLDisplay display;
static EGLConfig config;
static EGLint num_config;
static EGLContext context;



static void *getpix(unsigned npix)
{
	uint32_t *pix = malloc(npix * 4 * 4);
	for (unsigned i = 0; i < npix; i++)
		pix[i] = i;
	return pix;
}

static void getfmt(const char *format, GLenum *fmt, GLenum *ifmt, GLenum *type)
{
	if (!strcmp(format, "rgba32f")) {
		*fmt  = GL_RGBA;
		*ifmt = GL_RGBA32F;
		*type = GL_FLOAT;
	} else if (!strcmp(format, "rgba16f")) {
		*fmt  = GL_RGBA;
		*ifmt = GL_RGBA16F;
		*type = GL_HALF_FLOAT;
	} else if (!strcmp(format, "r32f")) {
		*fmt  = GL_RED;
		*ifmt = GL_R32F;
		*type = GL_FLOAT;
	} else if (!strcmp(format, "rgba32ui")) {
		*fmt  = GL_RGBA_INTEGER;
		*ifmt = GL_RGBA32UI;
		*type = GL_UNSIGNED_INT;
	} else if (!strcmp(format, "rgba16ui")) {
		*fmt  = GL_RGBA_INTEGER;
		*ifmt = GL_RGBA16UI;
		*type = GL_UNSIGNED_SHORT;
	} else if (!strcmp(format, "rgba8ui")) {
		*fmt  = GL_RGBA_INTEGER;
		*ifmt = GL_RGBA8UI;
		*type = GL_UNSIGNED_BYTE;
	} else if (!strcmp(format, "r32ui")) {
		*fmt  = GL_RED_INTEGER;
		*ifmt = GL_R32UI;
		*type = GL_UNSIGNED_INT;
	} else if (!strcmp(format, "rgba32i")) {
		*fmt  = GL_RGBA_INTEGER;
		*ifmt = GL_RGBA32I;
		*type = GL_INT;
	} else if (!strcmp(format, "rgba16i")) {
		*fmt  = GL_RGBA_INTEGER;
		*ifmt = GL_RGBA16I;
		*type = GL_SHORT;
	} else if (!strcmp(format, "rgba8i")) {
		*fmt  = GL_RGBA_INTEGER;
		*ifmt = GL_RGBA8I;
		*type = GL_BYTE;
	} else if (!strcmp(format, "r32i")) {
		*fmt  = GL_RED_INTEGER;
		*ifmt = GL_R32I;
		*type = GL_INT;
	} else if (!strcmp(format, "rgba8")) {
		*fmt  = GL_RGBA;
		*ifmt = GL_RGBA8;
		*type = GL_UNSIGNED_BYTE;
	} else if (!strcmp(format, "rgba8_snorm")) {
		*fmt  = GL_RGBA;
		*ifmt = GL_RGBA8_SNORM;
		*type = GL_BYTE;
	}
}

static int setup_texbuf(int program, const char *name, const char *format,
		int unit, int w)
{
	int handle;
	GLuint tex;

	handle = glGetUniformLocation(program, name);
	if (handle >= 0) {
		GLenum fmt, ifmt, type;

		getfmt(format, &fmt, &ifmt, &type);

		DEBUG_MSG("setup %s: fmt=%s, ifmt=%s, type=%s", name,
			formatname(fmt), formatname(ifmt), typename(type));

		GCHK(glGenTextures(1, &tex));

		GCHK(glActiveTexture(GL_TEXTURE0 + unit));
		GCHK(glBindTexture(GL_TEXTURE_BUFFER, tex));
//		GCHK(glTexStorage1D(GL_TEXTURE_BUFFER, 1, ifmt, w));

		GCHK(glBindImageTexture(unit, tex, 0, GL_FALSE, 0, GL_READ_WRITE, ifmt));

		unit++;
	}

	return unit;
}

static int setup_tex2d(int program, const char *name, const char *format,
		int unit, int w, int h)
{
	int handle;
	GLuint tex;

	handle = glGetUniformLocation(program, name);
	if (handle >= 0) {
		GLenum fmt, ifmt, type;

		getfmt(format, &fmt, &ifmt, &type);

		DEBUG_MSG("setup %s: fmt=%s, ifmt=%s, type=%s", name,
			formatname(fmt), formatname(ifmt), typename(type));

		GCHK(glGenTextures(1, &tex));

		GCHK(glActiveTexture(GL_TEXTURE0 + unit));
		GCHK(glBindTexture(GL_TEXTURE_2D, tex));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GCHK(glTexStorage2D(GL_TEXTURE_2D, 1, ifmt, w, h));
		GCHK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, fmt, type, getpix(w * h * 4)));

		GCHK(glBindImageTexture(unit, tex, 0, GL_FALSE, 0, GL_READ_WRITE, ifmt));

		unit++;
	}

	return unit;
}

static int setup_tex3d(int program, const char *name, const char *format,
		int unit, int w, int h, int d)
{
	int handle;
	GLuint tex;

	handle = glGetUniformLocation(program, name);
	if (handle >= 0) {
		GLenum fmt, ifmt, type;

		getfmt(format, &fmt, &ifmt, &type);

		DEBUG_MSG("setup %s: fmt=%s, ifmt=%s, type=%s", name,
			formatname(fmt), formatname(ifmt), typename(type));

		GCHK(glGenTextures(1, &tex));

		GCHK(glActiveTexture(GL_TEXTURE0 + unit));
		GCHK(glBindTexture(GL_TEXTURE_3D, tex));
		GCHK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GCHK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GCHK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
		GCHK(glTexStorage3D(GL_TEXTURE_3D, 1, ifmt, w, h, d));
		GCHK(glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, w, h, d, fmt, type, getpix(w * h * d * 4)));

		GCHK(glBindImageTexture(unit, tex, 0, GL_TRUE, 0, GL_READ_WRITE, ifmt));

		unit++;
	}

	return unit;
}

static int setup_texture(GLint program, int unit, const char *name,
		const char *type, const char *format, int w, int h, int d)
{
	if (strstr(type, "imageBuffer") || strstr(type, "image1D"))
		return setup_texbuf(program, name, format, unit, w);
	if (strstr(type, "image2D"))
		return setup_tex2d(program, name, format, unit, w, h);
	else if (strstr(type, "image3D"))
		return setup_tex3d(program, name, format, unit, w, h, d);
	else
		return unit;  // TODO other types
}

static const char *shader_source_fmt =
	"#version 310 es                                                   \n"
	"#extension GL_EXT_texture_buffer : enable                         \n"
	"                                                                  \n"
	"precision highp float;                                            \n"
	"precision highp int;                                              \n"
	"                                                                  \n"
	"layout(local_size_x=4, local_size_y=2, local_size_z=1) in;        \n"
	"                                                                  \n"
	"uniform highp layout(%s) readonly  %s src;                        \n"
	"uniform highp layout(%s) writeonly %s dst;                        \n"
	"uniform %s scoord;                                                \n"
	"uniform %s dcoord;                                                \n"
	"                                                                  \n"
	"void main(void) {                                                 \n"
	"    uint index = gl_LocalInvocationIndex;                         \n"
	"    imageStore(dst, dcoord + %s(index),                           \n"
	"               imageLoad(src, scoord + %s(index)));               \n"
	"}                                                                 \n"
	"\n";

static const char *shader_source_atomic_fmt =
	"#version 310 es                                                   \n"
	"#extension GL_EXT_texture_buffer : enable                         \n"
	"#extension GL_OES_shader_image_atomic : enable                    \n"
	"                                                                  \n"
	"precision highp float;                                            \n"
	"precision highp int;                                              \n"
	"                                                                  \n"
	"layout(local_size_x=4, local_size_y=2, local_size_z=1) in;        \n"
	"                                                                  \n"
	"uniform highp layout(%s) %s src;                                  \n"
	"uniform highp layout(%s) writeonly %s dst;                        \n"
	"/*uniform %s dcoord;*/                                            \n"
	"uniform %s scoord;                                                \n"
	"                                                                  \n"
	"void main(void) {                                                 \n"
	"    uint index = gl_LocalInvocationIndex;                         \n"
	"    imageAtomicAdd(src, scoord + %s(index), %s(7));               \n"
	"}                                                                 \n"
	"\n";

static const char * shader_source(int atomic,
		const char *src_fmt, const char *src_type, const char *src_coord,
		const char *dst_fmt, const char *dst_type, const char *dst_coord)
{
	char *source;
	asprintf(&source,
			atomic ? shader_source_atomic_fmt : shader_source_fmt,
			src_fmt, src_type,
			dst_fmt, dst_type,
			src_coord, dst_coord,
			dst_coord, src_coord);
	return source;
}

#ifndef GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS 0x90D7
#endif
#ifndef GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS 0x90D8
#endif
#ifndef GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS 0x90D9
#endif

#ifndef GL_MAX_GEOMETRY_IMAGE_UNIFORMS
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS    0x90CD
#endif
#ifndef GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS 0x90CB
#endif
#ifndef GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS 0x90CC
#endif

static const struct {
	const char *str;
	GLenum glenum;
#define ENUM(x) { #x, x }
} enums[] = {
	ENUM(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS),
	ENUM(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT),
	ENUM(GL_MAX_SHADER_STORAGE_BLOCK_SIZE),

	ENUM(GL_MAX_VERTEX_IMAGE_UNIFORMS),
	ENUM(GL_MAX_GEOMETRY_IMAGE_UNIFORMS),
	ENUM(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS),
	ENUM(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS),
	ENUM(GL_MAX_FRAGMENT_IMAGE_UNIFORMS),
	ENUM(GL_MAX_COMPUTE_IMAGE_UNIFORMS),
	ENUM(GL_MAX_COMBINED_IMAGE_UNIFORMS),

	ENUM(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS),
	ENUM(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE),
};

static void test_image(int atomic, int w, int h, int d,
		const char *src_fmt, const char *src_type, const char *src_coord,
		const char *dst_fmt, const char *dst_type, const char *dst_coord)
{
	int unit = 0;

	RD_START("image", "atomic=%d, w=%d, h=%d, d=%d, "
			"src_fmt=%s, src_type=%s, src_coord=%s, "
			"dst_fmt=%s, dst_type=%s, dst_coord=%s",
			atomic, w, h, d,
			src_fmt, src_type, src_coord,
			dst_fmt, dst_type, dst_coord);

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));
	ECHK(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context));

	DEBUG_MSG("EGL Version %s", eglQueryString(display, EGL_VERSION));
	DEBUG_MSG("EGL Vendor %s", eglQueryString(display, EGL_VENDOR));
	DEBUG_MSG("EGL Extensions %s", eglQueryString(display, EGL_EXTENSIONS));
	DEBUG_MSG("GL Version %s", glGetString(GL_VERSION));
	DEBUG_MSG("GL extensions: %s", glGetString(GL_EXTENSIONS));

	/* print some compute limits (not strictly necessary) */
	GLint work_group_count[3] = {0};
	for (unsigned i = 0; i < 3; i++)
		GCHK(glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &work_group_count[i]));
	DEBUG_MSG("GL_MAX_COMPUTE_WORK_GROUP_COUNT: %d, %d, %d",
			work_group_count[0],
			work_group_count[1],
			work_group_count[2]);

	GLint work_group_size[3] = {0};
	for (unsigned i = 0; i < 3; i++)
		GCHK(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &work_group_size[i]));
	DEBUG_MSG("GL_MAX_COMPUTE_WORK_GROUP_SIZE: %d, %d, %d",
			work_group_size[0],
			work_group_size[1],
			work_group_size[2]);

	GLint val, i;
	for (i = 0; i < ARRAY_SIZE(enums); i++) {
		glGetIntegerv(enums[i].glenum, &val);
		DEBUG_MSG("%s: %d", enums[i].str, val);
	}

	/* setup a compute shader */

	GLuint program = get_compute_program(shader_source(atomic,
			src_fmt, src_type, src_coord, dst_fmt, dst_type, dst_coord));

	link_program(program);

	unit = setup_texture(program, unit, "src", src_type, src_fmt, w, h, d);
	unit = setup_texture(program, unit, "dst", dst_type, dst_fmt, w, h, d);

	/* dispatch computation */
	PFNGLDISPATCHCOMPUTEPROC glDispatchCompute =
		(PFNGLDISPATCHCOMPUTEPROC)eglGetProcAddress("glDispatchCompute");
	GCHK(glDispatchCompute(2, 2, 2));

	DEBUG_MSG("Compute shader dispatched and finished successfully\n");

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	/* note: cube should be same thing as 3d, where one dimension is hard coded to 6,
	 * so we can ignore that.  1d/buffer *might* be same as 2d, similar to textures.
	 */
	TEST_START();

	/* 0 */
	TEST(test_image(0,  16,   16,   0, "rgba32ui",    "uimage2D", "ivec2", "rgba32ui",    "uimage2D", "ivec2"));
	TEST(test_image(0,  16,   16,   0, "rgba32i",     "iimage2D", "ivec2", "rgba32i",     "iimage2D", "ivec2"));
	TEST(test_image(0,  16,   16,   0, "rgba32f",     "image2D",  "ivec2", "rgba32f",     "image2D",  "ivec2"));
	TEST(test_image(0,  16,   16,  16, "rgba32f",     "image3D",  "ivec3", "rgba32f",     "image3D",  "ivec3"));
	TEST(test_image(0,  16,   16,   0, "rgba8",       "image2D",  "ivec2", "rgba32f",     "image2D",  "ivec2"));
	TEST(test_image(0,  16,   16,   0, "rgba32f",     "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,  16,   16,   0, "rgba8_snorm", "image2D",  "ivec2", "rgba32f",     "image2D",  "ivec2"));
	TEST(test_image(0,  16,   16,   0, "rgba32f",     "image2D",  "ivec2", "rgba8_snorm", "image2D",  "ivec2"));

	/* 8 */
	TEST(test_image(0,   1,    1,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,  16,    1,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,  32,    1,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,  64,    1,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,  65,    1,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,   1,   16,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,   1,   32,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));
	TEST(test_image(0,   1,   64,   0, "rgba8",       "image2D",  "ivec2", "rgba8",       "image2D",  "ivec2"));

	/* 16 */
	TEST(test_image(0,   1,    1,   1, "rgba8",       "image3D",  "ivec3", "rgba8",       "image3D",  "ivec3"));
	TEST(test_image(0,   1,    1,  16, "rgba8",       "image3D",  "ivec3", "rgba8",       "image3D",  "ivec3"));
	TEST(test_image(0,  16,   16,  16, "rgba8",       "image3D",  "ivec3", "rgba8",       "image3D",  "ivec3"));
	TEST(test_image(0,  65,  128,   0, "r32ui",       "uimage2D", "ivec2", "r32ui",       "uimage2D", "ivec2"));
	TEST(test_image(0,  65,  128,   0, "rgba32ui",    "uimage2D", "ivec2", "rgba32ui",    "uimage2D", "ivec2"));
	TEST(test_image(0,  65,  128,   0, "rgba16ui",    "uimage2D", "ivec2", "rgba16ui",    "uimage2D", "ivec2"));
	TEST(test_image(0,  65,  128,   0, "rgba8ui",     "uimage2D", "ivec2", "rgba8ui",     "uimage2D", "ivec2"));
	TEST(test_image(0,  65,  128,  65, "rgba8ui",     "uimage3D", "ivec3", "rgba8ui",     "uimage3D", "ivec3"));

	/* 24 */
	TEST(test_image(0, 8192,   0,   0, "r32ui", "uimageBuffer", "int",  "r32ui", "uimageBuffer", "int"));
	TEST(test_image(1, 8192,   0,   0, "r32ui", "uimageBuffer", "uint", "r32ui", "uimageBuffer", "int"));
	TEST(test_image(0,   65,  22,   0, "r32ui", "uimage2D",   "ivec2",  "r32ui", "uimage2D",   "ivec2"));
	TEST(test_image(1,   65,  22,   0, "r32ui", "uimage2D",    "uint",  "r32ui", "uimage2D",   "ivec2"));
	TEST(test_image(0,   65,  22,  11, "r32ui", "uimage3D",   "ivec3",  "r32ui", "uimage3D",   "ivec3"));
	TEST(test_image(1,   65,  22,  11, "r32ui", "uimage3D",    "uint",  "r32ui", "uimage3D",   "ivec3"));

	TEST_END();

	return 0;
}

