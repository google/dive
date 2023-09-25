/*
 * Copyright (c) 2021 Rob Clark <robdclark@gmail.com>
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
static GLuint program;

static const char *vs_shader_template =
	"#version 320 es                                                 \n"
	"#define MAX_VARYINGS %u                                         \n"
	"                                                                \n"
	"in vec4 aPosition;                                              \n"
	"                                                                \n"
	"out Data {                                                      \n"
	"  ivec4 f[MAX_VARYINGS];                                        \n"
	"} data;                                                         \n"
	"                                                                \n"
	"void main()                                                     \n"
	"{                                                               \n"
	"  gl_Position = aPosition;                                      \n"
	"                                                                \n"
	"  for (int i = 0; i < MAX_VARYINGS; i++)                        \n"
	"    data.f[i] = ivec4(i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3);  \n"
	"}                                                               \n"
	;

static char * get_vs(unsigned nvec4)
{
	static char buf[409600];
	sprintf(buf, vs_shader_template, nvec4);
	return buf;
}

static const char *tcs_shader_template =
	"#version 320 es                                                                        \n"
	"#define MAX_VARYINGS %u                                                                \n"
	"#define VERTICES     %u                                                                \n"
	"                                                                                       \n"
	"layout(vertices = VERTICES) out;                                                       \n"
	"                                                                                       \n"
	"in Data {                                                                              \n"
	"  ivec4 f[MAX_VARYINGS];                                                               \n"
	"} data[];                                                                              \n"
	"                                                                                       \n"
	"patch out vec4 tcs_color;                                                              \n"
	"                                                                                       \n"
	"void main() {                                                                          \n"
	"  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;            \n"
	"  gl_TessLevelOuter = float[4](1.0, 1.0, 1.0, 0.0);                                    \n"
	"  gl_TessLevelInner = float[2](0.0, 0.0);                                              \n"
	"                                                                                       \n"
	"  bool ok = true;                                                                      \n"
	"  for (int i = 0; i < MAX_VARYINGS; i++) {                                             \n"
	"	if (data[gl_InvocationID].f[i] != ivec4(i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3))    \n"
	"	  ok = false;                                                                       \n"
	"  }                                                                                    \n"
	"                                                                                       \n"
	"  tcs_color = ok ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);                \n"
	"}                                                                                      \n"
	;

static char * get_tcs(unsigned nvec4, unsigned vertices)
{
	static char buf[409600];
	sprintf(buf, tcs_shader_template, nvec4, vertices);
	return buf;
}

static const char *tes_shader_source =
	"#version 320 es                                            \n"
	"                                                           \n"
	"layout(triangles) in;                                      \n"
	"                                                           \n"
	"patch in vec4 tcs_color;                                   \n"
	"flat out vec4 color;                                       \n"
	"                                                           \n"
	"void main() {                                              \n"
	"	gl_Position = gl_in[0].gl_Position * gl_TessCoord[0]    \n"
	"				+ gl_in[1].gl_Position * gl_TessCoord[1]    \n"
	"				+ gl_in[2].gl_Position * gl_TessCoord[2];   \n"
	"                                                           \n"
	"	color = tcs_color;                                      \n"
	"}                                                          \n"
	;

static const char *fs_shader_source =
	"#version 320 es                           \n"
	"                                          \n"
	"precision highp float;                    \n"
	"precision highp int;                      \n"
	"                                          \n"
	"flat in vec4 color;                       \n"
	"out vec4 color_out;                       \n"
	"                                          \n"
	"void main()                               \n"
	"{                                         \n"
	"  color_out = color;                      \n"
	"}                                         \n"
	;

static void compile_shader(GLint program, const char *source, const char *stage_name, GLenum type)
{
	GLuint shader;
	GLint ret;

	DEBUG_MSG("%s shader:\n%s", stage_name, source);

	GCHK(shader = glCreateShader(type));
	GCHK(glShaderSource(shader, 1, &source, NULL));
	GCHK(glCompileShader(shader));
	GCHK(glGetShaderiv(shader, GL_COMPILE_STATUS, &ret));
	DEBUG_MSG("ret=%d", ret);
	if (!ret) {
		char *log;

		ERROR_MSG("%s shader compilation failed!:", stage_name);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(shader, ret, NULL, log);
			printf("%s", log);
		}
		exit(-1);
	}

	DEBUG_MSG("%s shader compilation succeeded!", stage_name);

	GCHK(glAttachShader(program, shader));
}

static void test(unsigned nvec4, unsigned vertices)
{
	GLint width, height;
	GLfloat vVertices[] = {
			// front
			-0.45, -0.75, 0.0,
			 0.45, -0.75, 0.0,
			-0.45,  0.75, 0.0,
			 0.45,  0.75, 0.0
	};
	EGLSurface surface;

	RD_START("tess-varyings", "nvec4=%u, vertices=%u", nvec4, vertices);

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

	program = get_program(get_vs(nvec4), fs_shader_source);
	compile_shader(program, get_tcs(nvec4, vertices), "tcs", GL_TESS_CONTROL_SHADER);
	compile_shader(program, tes_shader_source, "tes", GL_TESS_EVALUATION_SHADER);

	GCHK(glBindAttribLocation(program, 0, "aPosition"));

	link_program(program);

	GCHK(glViewport(0, 0, width, height));

	/* clear the color buffer */
//	GCHK(glClearColor(0.5, 0.5, 0.5, 1.0));
//	GCHK(glClear(GL_COLOR_BUFFER_BIT));

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices));
	GCHK(glEnableVertexAttribArray(0));

	GCHK(glDrawArrays(GL_PATCHES, 0, 4));

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
	for (unsigned vertices = 1; vertices <= 4; vertices++) {
		for (unsigned nvec4 = 1; nvec4 <= 31; nvec4++) {
			TEST(test(nvec4, vertices));
		}
	}
	TEST_END();

	return 0;
}

