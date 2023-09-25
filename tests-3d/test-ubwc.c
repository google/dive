/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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

/* Code copied from triangle_quad test from lima driver project adapted to the
 * logging that I use..
 *
 * this one is similar to test-quad-flat but the parameter that is varied is
 * the pbuffer size, to observe how the driver splits up rendering of different
 * sizes when GMEM overflows..
 */

#include <GLES3/gl31.h>
#include <assert.h>

#define GL_BGRA_EXT                                             0x80E1
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT                       0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT                       0x8366

#include "test-util-3d.h"

static EGLint const config_attribute_list[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_NONE
};

static const EGLint context_attribute_list[] = {
	EGL_CONTEXT_CLIENT_VERSION, 3,
	EGL_NONE
};

static EGLDisplay display;
static EGLConfig config;
static EGLint num_config;
static EGLContext context;
static GLuint program;
static int uniform_location, texture_handle;
const char *vertex_shader_source =
	"#version 300 es              \n"
	"in vec4 aPosition;           \n"
	"                             \n"
	"void main()                  \n"
	"{                            \n"
	"    gl_Position = aPosition; \n"
	"}                            \n";
const char *fragment_shader_source =
	"#version 300 es              \n"
	"precision highp float;       \n"
	"uniform vec4 uColor;         \n"
	"out vec4 col0;               \n"
	"                             \n"
	"void main()                  \n"
	"{                            \n"
	"    col0 = uColor;           \n"
	"}                            \n";

const char *vertex_shader_source_sam =
	"#version 300 es              \n"
	"in vec4 aPosition;           \n"
	"out vec2 vTexCoord;          \n"
	"                             \n"
	"void main()                  \n"
	"{                            \n"
	"    gl_Position = aPosition; \n"
	"    vTexCoord = aPosition.xy + vec2(0.1, 0.1);\n"
	"}                            \n";
const char *fragment_shader_source_sam =
	"#version 300 es              \n"
	"precision mediump float;     \n"
	"                             \n"
	"uniform sampler2D uTexture;  \n"
	"in vec2 vTexCoord;           \n"
	"out vec4 gl_FragColor;       \n"
	"                             \n"
	"void main()                  \n"
	"{                            \n"
	"    gl_FragColor = texture(uTexture, vTexCoord) * vec4(0.1, 0.2, 0.3, 0.4);\n"
	"}                            \n";


/* Run through multiple variants to detect mrt settings
 */
static void test(unsigned w, unsigned h, unsigned cpp, GLenum ifmt, GLenum fmt, GLenum type, int mipmap)
{
	GLint width, height;
	GLuint fbo, fbotex;
	GLenum mrt_bufs[] = {GL_COLOR_ATTACHMENT0};

	GLfloat quad_color[] =  {1.0, 0.0, 0.0, 1.0};
	GLfloat vertices[] = {
			-0.45, -0.75, 0.1,
			 0.45, -0.75, 0.1,
			-0.45,  0.75, 0.1,
			 0.45,  0.75, 0.1 };
	EGLSurface surface;

	////////////////////////////////////////////
	// calculate ubwc layout, see:
	// https://android.googlesource.com/platform/hardware/qcom/display/+/master/msm8996/libgralloc/alloc_controller.cpp#1057
	unsigned block_width, block_height;

	switch (cpp) {
	case 2:
	case 4:
		block_width = 16;
		block_height = 4;
		break;
	case 8:
		block_width = 8;
		block_height = 4;
		break;
	case 16:
		block_width = 4;
		block_height = 4;
		break;
	default:
		DEBUG_MSG("invalid cpp: %u", cpp);
		return;
	}

	// div_round_up():
	unsigned aligned_height = (h + block_height - 1) / block_height;
	unsigned aligned_width  = (w + block_width - 1) / block_width;

	// Align meta buffer height to 16 blocks
	unsigned meta_height = ALIGN(aligned_height, 16);

	// Align meta buffer width to 64 blocks
	unsigned meta_width = ALIGN(aligned_width, 64);

	// Align meta buffer size to 4K
	unsigned meta_size = ALIGN((meta_width * meta_height), 4096);

	////////////////////////////////////////////

	RD_START(mipmap ? "ubwc-layout-mipmap" : "ubwc-layout",
			"%dx%d, ifmt=%s, fmt=%s, type=%s, meta=%ux%u@0x%x (%ux%u)", w, h,
			formatname(fmt), formatname(ifmt), typename(type),
			meta_width, meta_height, meta_size, aligned_width, aligned_height);

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	surface = make_window(display, config, w, h);

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("Buffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));

	program = get_program(vertex_shader_source, fragment_shader_source);
	GCHK(glBindAttribLocation(program, 0, "aPosition"));
	link_program(program);

	GCHK(glGenFramebuffers(1, &fbo));
	GCHK(glGenTextures(1, &fbotex));
	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

	GCHK(glBindTexture(GL_TEXTURE_2D, fbotex));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GCHK(glTexImage2D(GL_TEXTURE_2D, 0, ifmt, width, height, 0, fmt, type, 0));
	GCHK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex, 0));

	DEBUG_MSG("status=%04x", glCheckFramebufferStatus(GL_FRAMEBUFFER));

	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

	GCHK(glViewport(0, 0, width, height));

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices));
	GCHK(glEnableVertexAttribArray(0));

	/* now set up our uniform. */
	GCHK(uniform_location = glGetUniformLocation(program, "uColor"));

	GCHK(glDrawBuffers(1, mrt_bufs));

//	glClearColor(0.25, 0.5, 0.75, 1.0);
//	GCHK(glClear(GL_COLOR_BUFFER_BIT));

	GCHK(glUniform4fv(uniform_location, 1, quad_color));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	/* clear any errors, in case it wasn't a renderable format: */
	while (glGetError() != GL_NO_ERROR) {}

	GCHK(glFlush());

	/* switch back to back buffer: */
	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	program = get_program(vertex_shader_source_sam, fragment_shader_source_sam);
	GCHK(glBindAttribLocation(program, 0, "aPosition"));
	link_program(program);

	GCHK(glActiveTexture(GL_TEXTURE0));
	GCHK(glBindTexture(GL_TEXTURE_2D, fbotex));

	if (mipmap) {
		GCHK(glGenerateMipmap(GL_TEXTURE_2D));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	} else {
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	}
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT));

	GCHK(texture_handle = glGetUniformLocation(program, "uTexture"));
	GCHK(glUniform1i(texture_handle, 0)); /* '0' refers to texture unit 0. */

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices));
	GCHK(glEnableVertexAttribArray(0));

	GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

	ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	static const struct {
		int cpp;
		GLenum ifmt;
		GLenum fmt;
		GLenum type;
	} fmts[] = {
#if 1
			{ 2, GL_RG8,        GL_RG,            GL_UNSIGNED_BYTE },
//			{ 3, GL_RGB8,       GL_RGB,           GL_UNSIGNED_BYTE },
			{ 4, GL_RGBA8,      GL_RGBA,          GL_UNSIGNED_BYTE },
			{ 2, GL_R16F,       GL_RED,           GL_FLOAT },
			{ 4, GL_RG16F,      GL_RG,            GL_FLOAT },
#else
			{ GL_R8,            GL_RED,           GL_UNSIGNED_BYTE },
			{ GL_R8UI,          GL_RED_INTEGER,   GL_UNSIGNED_BYTE },
			{ GL_R8I,           GL_RED_INTEGER,   GL_BYTE },
			{ GL_R16UI,         GL_RED_INTEGER,   GL_UNSIGNED_SHORT },
			{ GL_R16I,          GL_RED_INTEGER,   GL_SHORT },
			{ GL_R32UI,         GL_RED_INTEGER,   GL_UNSIGNED_INT },
			{ GL_R32I,          GL_RED_INTEGER,   GL_INT },
			{ GL_RG8,           GL_RG,            GL_UNSIGNED_BYTE },
			{ GL_RG8UI,         GL_RG_INTEGER,    GL_UNSIGNED_BYTE },
			{ GL_RG8I,          GL_RG_INTEGER,    GL_BYTE },
			{ GL_RG16UI,        GL_RG_INTEGER,    GL_UNSIGNED_SHORT },
			{ GL_RG16I,         GL_RG_INTEGER,    GL_SHORT },
			{ GL_RG32UI,        GL_RG_INTEGER,    GL_UNSIGNED_INT },
			{ GL_RG32I,         GL_RG_INTEGER,    GL_INT },
			{ GL_RGB8,          GL_RGB,           GL_UNSIGNED_BYTE },
			{ GL_RGB565,        GL_RGB,           GL_UNSIGNED_BYTE },
			{ GL_RGB565,        GL_RGB,           GL_UNSIGNED_SHORT_5_6_5 },
			{ GL_RGBA8,         GL_RGBA,          GL_UNSIGNED_BYTE },
			{ GL_SRGB8_ALPHA8,  GL_RGBA,          GL_UNSIGNED_BYTE },
			{ GL_RGB5_A1,       GL_RGBA,          GL_UNSIGNED_BYTE },
			{ GL_RGB5_A1,       GL_RGBA,          GL_UNSIGNED_SHORT_5_5_5_1 },
			{ GL_RGB5_A1,       GL_RGBA,          GL_UNSIGNED_INT_2_10_10_10_REV },
			{ GL_RGBA4,         GL_RGBA,          GL_UNSIGNED_BYTE },
			{ GL_RGBA4,         GL_RGBA,          GL_UNSIGNED_SHORT_4_4_4_4 },
			{ GL_RGB10_A2,      GL_RGBA,          GL_UNSIGNED_INT_2_10_10_10_REV },
			{ GL_RGBA8UI,       GL_RGBA_INTEGER,  GL_UNSIGNED_BYTE },
			{ GL_RGBA8I,        GL_RGBA_INTEGER,  GL_BYTE },
			{ GL_RGB10_A2UI,    GL_RGBA_INTEGER,  GL_UNSIGNED_INT_2_10_10_10_REV },
			{ GL_RGBA16UI,      GL_RGBA_INTEGER,  GL_UNSIGNED_SHORT },
			{ GL_RGBA16I,       GL_RGBA_INTEGER,  GL_SHORT },
			{ GL_RGBA32I,       GL_RGBA_INTEGER,  GL_INT },
			{ GL_RGBA32UI,      GL_RGBA_INTEGER,  GL_UNSIGNED_INT },
			/* Not required to be color renderable: */
			{ GL_R8_SNORM,       GL_RED,          GL_BYTE },
			{ GL_R16F,           GL_RED,          GL_HALF_FLOAT },
			{ GL_R16F,           GL_RED,          GL_FLOAT },
			{ GL_R32F,           GL_RED,          GL_FLOAT },
			{ GL_RG8_SNORM,      GL_RG,           GL_BYTE },
			{ GL_RG16F,          GL_RG,           GL_HALF_FLOAT },
			{ GL_RG16F,          GL_RG,           GL_FLOAT },
			{ GL_RG32F,          GL_RG,           GL_FLOAT },
			{ GL_SRGB8,          GL_RGB,          GL_UNSIGNED_BYTE },
			{ GL_RGB8_SNORM,     GL_RGB,          GL_BYTE },
			{ GL_R11F_G11F_B10F, GL_RGB,          GL_UNSIGNED_INT_10F_11F_11F_REV },
			{ GL_R11F_G11F_B10F, GL_RGB,          GL_HALF_FLOAT },
			{ GL_R11F_G11F_B10F, GL_RGB,          GL_FLOAT },
			{ GL_RGB9_E5,        GL_RGB,          GL_UNSIGNED_INT_5_9_9_9_REV },
			{ GL_RGB9_E5,        GL_RGB,          GL_HALF_FLOAT },
			{ GL_RGB9_E5,        GL_RGB,          GL_FLOAT },
			{ GL_RGB16F,         GL_RGB,          GL_HALF_FLOAT },
			{ GL_RGB16F,         GL_RGB,          GL_FLOAT },
			{ GL_RGB32F,         GL_RGB,          GL_FLOAT },
			{ GL_RGB8UI,         GL_RGB_INTEGER,  GL_UNSIGNED_BYTE },
			{ GL_RGB8I,          GL_RGB_INTEGER,  GL_BYTE },
			{ GL_RGB16UI,        GL_RGB_INTEGER,  GL_UNSIGNED_SHORT },
			{ GL_RGB16I,         GL_RGB_INTEGER,  GL_SHORT },
			{ GL_RGB32UI,        GL_RGB_INTEGER,  GL_UNSIGNED_INT },
			{ GL_RGB32I,         GL_RGB_INTEGER,  GL_INT },
			{ GL_RGBA8_SNORM,    GL_RGBA,         GL_BYTE },
			{ GL_RGBA16F,        GL_RGBA,         GL_HALF_FLOAT },
			{ GL_RGBA16F,        GL_RGBA,         GL_FLOAT },
			{ GL_RGBA32F,        GL_RGBA,         GL_FLOAT },
#endif
	};
	static const struct {
		unsigned width, height;
	} sizes[] = {
			{   23,   23 },
			{   23,   34 },
			{   32,   23 },
			{  100,   34 },
			{  100,	 100 },
			{  100,  500 },
			{  500,  100 },
			{ 1000,	 100 },
			{ 1000, 1000 },
			{ 2000,  100 },
			{  100, 2000 },
			{ 2000, 2000 },
	};
	int i, j;

	TEST_START();
	for (i = 0; i < ARRAY_SIZE(fmts); i++) {
		for (j = 0; j < ARRAY_SIZE(sizes); j++) {
			TEST(test(sizes[j].width, sizes[j].height, fmts[i].cpp,
					fmts[i].ifmt, fmts[i].fmt, fmts[i].type, 0));
			TEST(test(sizes[j].width, sizes[j].height, fmts[i].cpp,
					fmts[i].ifmt, fmts[i].fmt, fmts[i].type, 1));
		}
	}
	TEST_END();

	return 0;
}

