/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
 * Copyright (c) 2012 Jonathan Maw <jonathan.maw@codethink.co.uk>
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
 * Copyright (c) 2014 Ilia Mirkin <imirkin@alum.mit.edu>
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
const char *vertex_shader_source =
		"#version 300 es              \n"
		"in vec4 in_position;         \n"
		"\n"
		"out vec4 pos;                \n"
		"out vec2 pos2;               \n"
		"out vec3 pos3;               \n"
		"out vec4 pos4;               \n"
		"out vec2 pos5;               \n"
		"out vec3 pos6;               \n"
		"out vec4 pos7;               \n"
		//              "out vec4 posen[5];          \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    pos = in_position;       \n"
		"    pos2 = in_position.xy;   \n"
		"    pos3 = in_position.xyz;  \n"
		"    pos4 = in_position.xyzw; \n"
		"    pos5 = in_position.yx;   \n"
		"    pos6 = in_position.yxz;  \n"
		"    pos7 = in_position.yxzw; \n"
		//              "    for (int i = 0; i < 5; i++)          \n"
		//              "        posen[i] = in_position + vec4(i); \n"
		"}                            \n";

const char *fragment_shader_source =
		"#version 300 es              \n"
		"precision highp float;       \n"
		"in vec4 pos;                 \n"
		"in vec2 pos2;                \n"
		"in vec3 pos3;                \n"
		"in vec4 pos4;                \n"
		"out vec4 gl_FragColor;       \n"
		"void main() {                \n"
		"gl_FragColor = vec4(1, 0, 0, 0);\n"
		"gl_FragColor.x += pos.x;     \n"
		"gl_FragColor.y += pos2.x;    \n"
		"gl_FragColor.z += pos3.x;    \n"
		"gl_FragColor.w += pos4.x;    \n"
		"} \n";



static void test_transform_feedback(int n, int separate)
{
	GLint width, height, ret, i;
	GLuint texturename = 0, texture_handle, tf;
	GLfloat vVertices[] = {
			// front
			-0.45, -0.75, 0.0,
			0.45, -0.75, 0.0,
			-0.45,  0.75, 0.0,
			0.45,  0.75, 0.0
	};

	const char *varyings[] = {
			"pos",
			"pos2",
			"pos3",
			"pos4",
			"pos5",
			"pos6",
			"pos7",
			//"posen[0]",
			//"posen[1]",
			//"posen[2]",
			//"posen[8]",
			//"posen[5]",
	};
	GLuint tf_bufs[ARRAY_SIZE(varyings)] = { 0 };

	EGLSurface surface;

	RD_START("transform-feedback", "n=%d, separate=%d", n, separate);

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	surface = make_window(display, config, 255, 255);

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("Buffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));

	printf("EGL Version %s\n", eglQueryString(display, EGL_VERSION));
	printf("EGL Vendor %s\n", eglQueryString(display, EGL_VENDOR));
	printf("EGL Extensions %s\n", eglQueryString(display, EGL_EXTENSIONS));
	printf("GL Version %s\n", glGetString(GL_VERSION));
	printf("GL extensions: %s\n", glGetString(GL_EXTENSIONS));

	program = get_program(vertex_shader_source, fragment_shader_source);

	if (n > 0)
		GCHK(glEnable(GL_RASTERIZER_DISCARD));

	GCHK(glBindAttribLocation(program, 0, "in_position"));

	if (n > 0) {
		GCHK(glTransformFeedbackVaryings(program, n, varyings,
				separate ? GL_SEPARATE_ATTRIBS : GL_INTERLEAVED_ATTRIBS));
	}

	link_program(program);

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices));
	GCHK(glEnableVertexAttribArray(0));

	if (n > 0) {
		GCHK(glGenBuffers(n, tf_bufs));
		for (i = 0; i < (separate ? n : 1); i++) {
			GCHK(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tf_bufs[i]));
			GCHK(glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1024, NULL, GL_STREAM_DRAW));
			GCHK(glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, i, tf_bufs[i], 32 * i, 1024));
		}
	}

	GLuint query;
	GCHK(glGenQueries(1, &query));

	//GCHK(glGenTransformFeedbacks(1, &tf));
	//GCHK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tf));

	if (n > 0) {
		GCHK(glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query));
		GCHK(glBeginTransformFeedback(GL_POINTS));
	}

	GCHK(glDrawArrays(GL_POINTS, 0, 4));

	if (n > 0) {
		GCHK(glEndTransformFeedback());
		GCHK(glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN));
	}

	//ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

	sleep(1);

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	int i;

	TEST_START();
	TEST(test_transform_feedback(0, 0));
	for (i = 1; i <= 4; i++) {
		TEST(test_transform_feedback(i, 0));
		TEST(test_transform_feedback(i, 1));
	}
	for (; i <= 7; i++) {
		TEST(test_transform_feedback(i, 0));
	}
	TEST_END();

	return 0;
}

