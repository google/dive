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

#include <GLES3/gl32.h>
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
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    pos = in_position;       \n"
		"}                            \n";

const char *fragment_shader_source =
		"#version 300 es              \n"
		"precision mediump float;     \n"
		"out vec4 gl_FragColor;       \n"
		"void main() {                \n"
		"gl_FragColor = vec4(1, 0, 0, 0);\n"
		"} \n";



static void test_draw(int n)
{
	GLint width, height, ret, i;
	GLuint texturename = 0, texture_handle, tf;
	GLfloat vVertices[64] = {
			// front
			-0.45, -0.75, 0.0,
			0.45, -0.75, 0.0,
			-0.45,  0.75, 0.0,
			0.45,  0.75, 0.0,
			-0.45, -0.75, 0.0,
			0.45, -0.75, 0.0,
			-0.45,  0.75, 0.0,
			0.45,  0.75, 0.0,
	};
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	static const char *testname[] = {
		"glDrawArrays", "glDrawArraysInstanced","glDrawArraysIndirect",
		"glDrawElements", "glDrawElementsInstancedBaseVertex", "glDrawElementsIndirect",
	};
	GLuint vbo, vao;

	EGLSurface surface;

	RD_START("draw-indirect", "%s", testname[n]);

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

	GCHK(glBindAttribLocation(program, 0, "in_position"));

	link_program(program);

	GCHK(glGenVertexArrays(1, &vao));
	GCHK(glBindVertexArray(vao));

	GCHK(glGenBuffers(1, &vbo));
	GCHK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GCHK(glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW));

	GCHK(glEnableVertexAttribArray(0));
	GCHK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL));

	if (n == 0) {
		GCHK(glDrawArrays(GL_TRIANGLES, 4, 8));
	} else if (n == 1) {
		GCHK(glDrawArraysInstanced(GL_TRIANGLES, 4, 8, 3));
	} else if (n == 2) {
		struct {
			uint32_t count;
			uint32_t prim_count;
			uint32_t first;
			uint32_t must_be_zero;
		} draw_arrays[5] = { 8, 3, 4, 0 };
		GLuint indirect_buffer;

		GCHK(glGenBuffers(1, &indirect_buffer));
		GCHK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_buffer));
		GCHK(glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(draw_arrays), &draw_arrays, GL_STATIC_DRAW));

		GCHK(glDrawArraysIndirect(GL_TRIANGLES, 0));
	} else if (n == 3) {
		GCHK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices));
	} else if (n == 4) {
		GCHK(glDrawElementsInstancedBaseVertex(GL_TRIANGLES, 8, GL_UNSIGNED_SHORT, indices, 3, 5));
	} else if (n == 5) {
		struct {
			uint32_t count;
			uint32_t instance_count;
			uint32_t first_index;
			int32_t  base_vertex;
			uint32_t must_be_zero;
		} draw_elements[5] = { 8, 3, 2, 5, 0 };
		GLuint indirect_buffer, index_buffer;

		GCHK(glGenBuffers(1, &indirect_buffer));
		GCHK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_buffer));
		GCHK(glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(draw_elements), &draw_elements, GL_STATIC_DRAW));

		GCHK(glGenBuffers(1, &index_buffer));
		GCHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer));
		GCHK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW));

		GCHK(glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0));
	}

	//ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

	ECHK(eglDestroySurface(display, surface));
	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	int i, j;

	TEST_START();
	TEST(test_draw(0));
	TEST(test_draw(1));
	TEST(test_draw(2));
	TEST(test_draw(3));
	TEST(test_draw(4));
	TEST(test_draw(5));
	TEST_END();

	return 0;
}

