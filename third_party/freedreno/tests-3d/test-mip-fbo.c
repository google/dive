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
void test(unsigned w, unsigned h, GLenum ifmt, GLenum fmt, GLenum type)
{
	GLint width, height, level = 0;
	GLuint fbo, fbotex;
	GLenum mrt_bufs[] = {GL_COLOR_ATTACHMENT0};

	GLfloat quad_color[] =  {1.0, 0.0, 0.0, 1.0};
	GLfloat vertices[] = {
			-0.45, -0.75, 0.1,
			 0.45, -0.75, 0.1,
			-0.45,  0.75, 0.1,
			 0.45,  0.75, 0.1 };
	EGLSurface surface;

	RD_START("mip-fbo", "%dx%d, ifmt=%s, fmt=%s, type=%s", w, h,
			formatname(fmt), formatname(ifmt), typename(type));

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

	while ((w > 0) && (h > 0)) {
		DEBUG_MSG("level=%d, %dx%d", level, w, h);

		GCHK(glBindTexture(GL_TEXTURE_2D, fbotex));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GCHK(glTexImage2D(GL_TEXTURE_2D, level, ifmt, w, h, 0, fmt, type, 0));
		GCHK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex, level));

		DEBUG_MSG("status=%04x", glCheckFramebufferStatus(GL_FRAMEBUFFER));

		GCHK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

		GCHK(glViewport(0, 0, w, h));

		GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices));
		GCHK(glEnableVertexAttribArray(0));

		/* now set up our uniform. */
		GCHK(uniform_location = glGetUniformLocation(program, "uColor"));

		GCHK(glDrawBuffers(1, mrt_bufs));

		glClearColor(0.25, 0.5, 0.75, 1.0);
		GCHK(glClear(GL_COLOR_BUFFER_BIT));
		GCHK(glFlush());

		/* for a5xx, encourage blob to use GMEM instead of BYPASS */
		GCHK(glEnable(GL_BLEND));

		GCHK(glUniform4fv(uniform_location, 1, quad_color));
		GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
		GCHK(glFlush());

		w >>= 1;
		h >>= 1;
		level++;
	}

	/* switch back to back buffer: */
	GCHK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	program = get_program(vertex_shader_source_sam, fragment_shader_source_sam);
	GCHK(glBindAttribLocation(program, 0, "aPosition"));
	link_program(program);

	GCHK(glActiveTexture(GL_TEXTURE0));
	GCHK(glBindTexture(GL_TEXTURE_2D, fbotex));

	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
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
	TEST_START();
	TEST(test(1024, 1024, GL_R8, GL_RED, GL_UNSIGNED_BYTE));
	TEST_END();

	return 0;
}

