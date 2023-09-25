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

#include <GLES3/gl31.h>
#include <assert.h>

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
const char *vertex_shader_source =
	"#version 310 es                        \n"
	"in vec4 aPosition;                     \n"
	"                                       \n"
	"void main()                            \n"
	"{                                      \n"
	"    gl_Position = aPosition;           \n"
	"}                                      \n";
const char *fragment_shader_source =
	"#version 310 es                        \n"
	"precision highp float;                 \n"
	"layout (std430) buffer Counter {       \n"
    "  uint counter[];                      \n"
    "} counter;                             \n"
	"                                       \n"
	"void main()                            \n"
	"{                                      \n"
	"    atomicAdd(counter.counter[3], 1u); \n"
	"}                                      \n";

static void test(int sz)
{
	GLint width, height;
	GLuint fbo, fbotex, query, ssbo;
	GLenum mrt_bufs[] = {GL_COLOR_ATTACHMENT0};
	GLfloat vertices[] = {
			-0.45, -0.75, 0.1,
			 0.45, -0.75, 0.1,
			-0.45,  0.75, 0.1,
			 0.45,  0.75, 0.1 };
	EGLSurface surface;
	uint32_t buf[sz];
	uint32_t *p;
	const int w = 160;
	const int h = 160;
	int idx;

	RD_START("ssbo", "sz=%d (0x%x)", sz, sz);

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	surface = make_window(display, config, 64, 64);

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("Buffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));

	DEBUG_MSG("EGL Version %s", eglQueryString(display, EGL_VERSION));
	DEBUG_MSG("EGL Vendor %s", eglQueryString(display, EGL_VENDOR));
	DEBUG_MSG("EGL Extensions %s", eglQueryString(display, EGL_EXTENSIONS));
	DEBUG_MSG("GL Version %s", glGetString(GL_VERSION));
	DEBUG_MSG("GL extensions: %s", glGetString(GL_EXTENSIONS));

	program = get_program(vertex_shader_source, fragment_shader_source);
	GCHK(glBindAttribLocation(program, 0, "aPosition"));
	link_program(program);

	GCHK(glGenFramebuffers(1, &fbo));
	GCHK(glGenTextures(1, &fbotex));
	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

	idx = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "Counter");

	GCHK(glGenBuffers(1, &ssbo));
	GCHK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo));
	GCHK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * sz, buf, GL_DYNAMIC_DRAW));

	GCHK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, idx, ssbo));

	GCHK(glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, w));
	GCHK(glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, h));
	DEBUG_MSG("status=%04x", glCheckFramebufferStatus(GL_FRAMEBUFFER));

	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

	GCHK(glViewport(0, 0, w, h));

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices));
	GCHK(glEnableVertexAttribArray(0));

	GCHK(glDrawBuffers(1, mrt_bufs));

//	GCHK(glClearColor(0.25, 0.5, 0.75, 1.0));
//	GCHK(glClear(GL_COLOR_BUFFER_BIT));

	GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

	GCHK(glFlush());

	GCHK(p = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * sz, GL_MAP_READ_BIT));
	DEBUG_MSG("counter: %u", p[3]);
	GCHK(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	TEST_START();
	TEST(test(  0x0004));
	TEST(test(  0x0010));
	TEST(test(  0x0040));
	TEST(test(  0x0100));
	TEST(test(  0x7fff));
	TEST(test( 0x1ffff));
	TEST(test( 0x2ffff));
	TEST(test( 0x3ffff));
	TEST(test( 0x4ffff));
	TEST(test( 0x5ffff));
	TEST(test(0x100000));
	TEST_END();

	return 0;
}

