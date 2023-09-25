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

/* Run through multiple variants to detect mrt settings
 */
static void test(unsigned w, unsigned h, GLenum ifmt, GLenum fmt, GLenum type, GLbitfield b)
{
	GLint width, height;
	GLuint fbo, fbotex;
	GLenum mrt_bufs[] = {GL_COLOR_ATTACHMENT0};
	EGLSurface surface;

	RD_START("clear-fbo", "%dx%d, ifmt=%s, fmt=%s, type=%s, b=%x", w, h,
			formatname(fmt), formatname(ifmt), typename(type), b);

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

	GCHK(glGenFramebuffers(1, &fbo));
	GCHK(glGenTextures(1, &fbotex));
	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

	GLenum target = GL_TEXTURE_2D;
	GCHK(glBindTexture(target, fbotex));
	GCHK(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GCHK(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GCHK(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GCHK(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GCHK(glTexImage2D(target, 0, ifmt, width, height, 0, fmt, type, 0));

	switch (fmt) {
	case GL_DEPTH_COMPONENT:
		GCHK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, fbotex, 0));
		break;
	case GL_DEPTH_STENCIL:
		GCHK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, target, fbotex, 0));
		break;
	default:
		GCHK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, fbotex, 0));
		break;
	}

	if ((fmt == GL_DEPTH_COMPONENT) || (fmt == GL_DEPTH_STENCIL)) {
		GLuint colortex;

		/* also create a dummy color attachment: */
		GCHK(glGenTextures(1, &colortex));
		GCHK(glBindTexture(target, colortex));
		GCHK(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GCHK(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GCHK(glTexImage2D(target, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, 0));

		GCHK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, colortex, 0));
	}

	DEBUG_MSG("status=%04x", glCheckFramebufferStatus(GL_FRAMEBUFFER));

	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

	GCHK(glViewport(0, 0, width, height));

	GCHK(glDrawBuffers(1, mrt_bufs));

	glClearColor(0.25, 0.5, 0.75, 1.0);
	GCHK(glClearStencil(0x88));
	GCHK(glClearDepthf(0.2));
	GCHK(glClear(b));
	GCHK(glFlush());
	ECHK(eglSwapBuffers(display, surface));

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	static const struct {
		GLenum ifmt;
		GLenum fmt;
		GLenum type;
	} fmts[] = {
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
#if 0
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
			{ GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT },
			{ GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT },
			{ GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT },
			{ GL_DEPTH24_STENCIL8,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8 },
			{ GL_DEPTH32F_STENCIL8,  GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV },
	};
	int i;

	TEST_START();
	for (i = 0; i < ARRAY_SIZE(fmts); i++) {
		if (fmts[i].fmt == GL_DEPTH_STENCIL) {
			TEST(test(128, 128, fmts[i].ifmt, fmts[i].fmt, fmts[i].type, GL_DEPTH_BUFFER_BIT));
			TEST(test(128, 128, fmts[i].ifmt, fmts[i].fmt, fmts[i].type, GL_STENCIL_BUFFER_BIT));
			TEST(test(128, 128, fmts[i].ifmt, fmts[i].fmt, fmts[i].type, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
		} else if (fmts[i].fmt == GL_DEPTH_COMPONENT) {
			TEST(test(128, 128, fmts[i].ifmt, fmts[i].fmt, fmts[i].type, GL_DEPTH_BUFFER_BIT));
		} else {
			TEST(test(128, 128, fmts[i].ifmt, fmts[i].fmt, fmts[i].type, GL_COLOR_BUFFER_BIT));
		}
	}
	TEST_END();

	return 0;
}

