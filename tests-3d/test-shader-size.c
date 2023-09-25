/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
 * Copyright (c) 2012 Jonathan Maw <jonathan.maw@codethink.co.uk>
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

/* Code based on cube_textured test from lima driver project (Jonathan Maw)
 * adapted to the logging that I use..
 */

#include <GLES3/gl3.h>
#include "test-util-3d.h"

#include "cubetex.h"

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
static GLuint program;

static char * get_vs(int cnt)
{
	static char buf[409600];
	char *ptr = buf;
	int i;

	ptr += sprintf(ptr, "attribute vec4 aPosition;\n");
	ptr += sprintf(ptr, "uniform sampler2D uTex;\n");
	ptr += sprintf(ptr, "varying vec4 vPosition;\n");
	ptr += sprintf(ptr, "void main()\n");
	ptr += sprintf(ptr, "{\n");
	ptr += sprintf(ptr, "  vec4 val = aPosition;\n");
	ptr += sprintf(ptr, "  vPosition = aPosition;\n");
	for (i = 0; i < cnt; i++)
		ptr += sprintf(ptr, "  val = texture2D(uTex, val.xy) * %f;\n", ((float)(i+1))/2.0);
	ptr += sprintf(ptr, "  gl_Position = val;\n");
	ptr += sprintf(ptr, "}\n");

	return buf;
}

static char * get_fs(int cnt)
{
	static char buf[409600];
	char *ptr = buf;
	int i, j;

	ptr += sprintf(ptr, "precision highp float;\n");
	ptr += sprintf(ptr, "uniform sampler2D uTex;\n");
	ptr += sprintf(ptr, "varying vec4 vPosition;\n");
	ptr += sprintf(ptr, "void main()\n");
	ptr += sprintf(ptr, "{\n");
	ptr += sprintf(ptr, "  vec4 val = vPosition;\n");
	for (i = 0; i < cnt; i++)
		ptr += sprintf(ptr, "  val = texture2D(uTex, val.xy) * %f;\n", ((float)(i+1))/2.0);
	ptr += sprintf(ptr, "  gl_FragColor = val;\n");
	ptr += sprintf(ptr, "}\n");

	return buf;
}



static void test(int vsz, int fsz)
{
	GLint width, height;
	GLuint tex, texture_handle;
	GLfloat vVertices[] = {
			// front
			-0.45, -0.75, 0.0,
			 0.45, -0.75, 0.0,
			-0.45,  0.75, 0.0,
			 0.45,  0.75, 0.0
	};

	GLfloat vTexCoords[] = {
			1.0f, 1.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			0.0f, 0.0f,
	};

	EGLSurface surface;

	RD_START("shader-size", "vsz=%d, fsz=%d", vsz, fsz);

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	surface = make_window(display, config, 512, 512);

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("Buffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));

	DEBUG_MSG("EGL Version %s\n", eglQueryString(display, EGL_VERSION));
	DEBUG_MSG("EGL Vendor %s\n", eglQueryString(display, EGL_VENDOR));
	DEBUG_MSG("EGL Extensions %s\n", eglQueryString(display, EGL_EXTENSIONS));
	DEBUG_MSG("GL Version %s\n", glGetString(GL_VERSION));
	DEBUG_MSG("GL extensions: %s\n", glGetString(GL_EXTENSIONS));

	program = get_program(get_vs(vsz), get_fs(fsz));

	GCHK(glBindAttribLocation(program, 0, "aPosition"));

	link_program(program);

	GCHK(glViewport(0, 0, width, height));

	/* clear the color buffer */
//	GCHK(glClearColor(0.5, 0.5, 0.5, 1.0));
//	GCHK(glClear(GL_COLOR_BUFFER_BIT));

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices));
	GCHK(glEnableVertexAttribArray(0));

	GCHK(glGenTextures(1, &tex));

	GCHK(glActiveTexture(GL_TEXTURE0));
	GCHK(glBindTexture(GL_TEXTURE_2D, tex));

	GCHK(glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGB,
			cube_texture.width, cube_texture.height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, cube_texture.pixel_data));

	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

	GCHK(texture_handle = glGetUniformLocation(program, "uTex"));
	GCHK(glUniform1i(texture_handle, 0)); /* '0' refers to texture unit 0. */

	GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

	ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

//	sleep(1);

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	TEST_START();
	TEST(test(0, 0));
	TEST(test(0, 1));
	TEST(test(1, 0));
	TEST(test(0, 2));
	TEST(test(2, 0));
	TEST(test(2, 2));
	TEST(test(2, 100));
	TEST(test(2, 200));
	TEST(test(2, 300));
	TEST(test(2, 600));
	TEST(test(2, 1200));
	TEST(test(100, 2));
	TEST(test(200, 2));
	TEST(test(300, 2));
	TEST(test(600, 2));
	TEST(test(1200, 2));
	TEST_END();

	return 0;
}

