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

/* Code copied from triangle_smoothed test from lima driver project adapted to the
 * logging that I use..
 */

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include "test-util-3d.h"

#ifndef GL_KHR_blend_equation_advanced
#define GL_KHR_blend_equation_advanced 1
#define GL_MULTIPLY_KHR                   0x9294
#define GL_SCREEN_KHR                     0x9295
#define GL_OVERLAY_KHR                    0x9296
#define GL_DARKEN_KHR                     0x9297
#define GL_LIGHTEN_KHR                    0x9298
#define GL_COLORDODGE_KHR                 0x9299
#define GL_COLORBURN_KHR                  0x929A
#define GL_HARDLIGHT_KHR                  0x929B
#define GL_SOFTLIGHT_KHR                  0x929C
#define GL_DIFFERENCE_KHR                 0x929E
#define GL_EXCLUSION_KHR                  0x92A0
#define GL_HSL_HUE_KHR                    0x92AD
#define GL_HSL_SATURATION_KHR             0x92AE
#define GL_HSL_COLOR_KHR                  0x92AF
#define GL_HSL_LUMINOSITY_KHR             0x92B0
typedef void (PFNGLBLENDBARRIERKHRPROC) (void);
#endif /* GL_KHR_blend_equation_advanced */

#ifndef GL_KHR_blend_equation_advanced_coherent
#define GL_KHR_blend_equation_advanced_coherent 1
#define GL_BLEND_ADVANCED_COHERENT_KHR    0x9285
#endif /* GL_KHR_blend_equation_advanced_coherent */

static const char * blendname(GLenum blend)
{
	switch (blend) {
	default:  break;
	case 0:   return "none";
#define ENUM(n) case GL_ ## n ## _KHR: return #n
	ENUM(MULTIPLY);
	ENUM(SCREEN);
	ENUM(OVERLAY);
	ENUM(DARKEN);
	ENUM(LIGHTEN);
	ENUM(COLORDODGE);
	ENUM(COLORBURN);
	ENUM(HARDLIGHT);
	ENUM(SOFTLIGHT);
	ENUM(DIFFERENCE);
	ENUM(EXCLUSION);
	ENUM(HSL_HUE);
	ENUM(HSL_SATURATION);
	ENUM(HSL_COLOR);
	ENUM(HSL_LUMINOSITY);
#undef ENUM
	}
	ERROR_MSG("invalid blend: %04x", blend);
	exit(1);
	return NULL;
}

static EGLint const config_attribute_list[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_DEPTH_SIZE, 8,
	EGL_NONE
};

static EGLint const config_attribute_list_msaa[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_DEPTH_SIZE, 8,
	EGL_SAMPLES, 4,
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
		"in vec4 aPosition;           \n"
		"in vec4 aColor;              \n"
		"out vec4 vColor;             \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    vColor = aColor;         \n"
		"    gl_Position = aPosition; \n"
		"}                            \n";

const char *fragment_shader_source =
		"#version 300 es              \n"
		"precision highp float;       \n"
		"                             \n"
		"in vec4 vColor;              \n"
		"out vec4 gl_FragColor;       \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = vColor;   \n"
		"}                            \n";

const char *fragment_shader_source_tex =
		"#version 300 es              \n"
		"precision highp float;       \n"
		"                             \n"
		"in vec4 vColor;              \n"
		"uniform sampler2D uTex2D0;   \n"
		"out vec4 gl_FragColor;       \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = texture(uTex2D0, vColor.xy);\n"
		"    gl_FragColor += vColor;  \n"
		"}                            \n";

const char *fragment_shader_source_ab =
		"#version 300 es              \n"
		"#extension GL_KHR_blend_equation_advanced : enable\n"
		"precision highp float;       \n"
		"                             \n"
		"in vec4 vColor;              \n"
		"layout(blend_support_all_equations) out vec4 gl_FragColor;\n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = vColor;   \n"
		"}                            \n";

const char *fragment_shader_source_ab_tex =
		"#version 300 es              \n"
		"#extension GL_KHR_blend_equation_advanced : enable\n"
		"precision highp float;       \n"
		"                             \n"
		"in vec4 vColor;              \n"
		"uniform sampler2D uTex2D0;   \n"
		"layout(blend_support_all_equations) out vec4 gl_FragColor;\n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = texture(uTex2D0, vColor.xy);\n"
		"    gl_FragColor += vColor;  \n"
		"}                            \n";

/* mode:
 *   0 - default, two draws
 *   1 - use glBlendBarrier() between the two draws
 *   2 - enable GL_BLEND_ADVANCED_COHERENT_KHR
 */

static void test_advanced_blend(int mode, GLenum blend)
{
	GLint width, height;
	GLint handle;

	GLfloat vVertices[] = {
			 0.0f,  0.5f, 0.0f,
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f };
	GLfloat vColors[] = {
			1.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f, 1.0f};
	EGLSurface surface;

	if (env2u("MSAA")) {
		RD_START("advanced-blend-msaa", "mode=%u, blend=%s", mode, blendname(blend));
	} else if (env2u("TEX")) {
		RD_START("advanced-blend-tex", "mode=%u, blend=%s", mode, blendname(blend));
	} else {
		RD_START("advanced-blend", "mode=%u, blend=%s", mode, blendname(blend));
	}

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	if (env2u("MSAA")) {
		ECHK(eglChooseConfig(display, config_attribute_list_msaa, &config, 1, &num_config));
	} else {
		ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	}
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	surface = make_window(display, config, 800, 600);

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("Buffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));

	if (env2u("TEX")) {
		program = get_program(vertex_shader_source,
				blend ? fragment_shader_source_ab_tex : fragment_shader_source_tex);
	} else {
		program = get_program(vertex_shader_source,
				blend ? fragment_shader_source_ab : fragment_shader_source);
	}

	GCHK(glBindAttribLocation(program, 0, "aPosition"));
	GCHK(glBindAttribLocation(program, 1, "aColor"));

	link_program(program);

	if (env2u("TEX")) {
		GLint handle;
		GLuint tex;

		GCHK(handle = glGetUniformLocation(program, "uTex2D0"));
		GCHK(glGenTextures(1, &tex));

		GCHK(glActiveTexture(GL_TEXTURE0));
		GCHK(glBindTexture(GL_TEXTURE_2D, tex));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 200, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, malloc(200 * 200 * 4)));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 1));
		GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4));

		GCHK(glUniform1i(handle, 0));
	}

	DEBUG_MSG("GL Version %s", glGetString(GL_VERSION));
	DEBUG_MSG("GL Extensions \"%s\"", glGetString(GL_EXTENSIONS));

	GCHK(glViewport(0, 0, width, height));

	/* clear the color buffer */
	GCHK(glClearColor(0.0, 0.0, 0.0, 1.0));
	GCHK(glEnable(GL_DEPTH_TEST));
	GCHK(glDepthFunc(GL_LEQUAL));
	GCHK(glEnable(GL_CULL_FACE));
	GCHK(glCullFace(GL_BACK));
	GCHK(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));

	if (blend) {
		GCHK(glBlendEquation(blend));
		GCHK(glEnable(GL_BLEND));
	} else {
		GCHK(glDisable(GL_BLEND));
	}

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices));
	GCHK(glEnableVertexAttribArray(0));

	GCHK(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, vColors));
	GCHK(glEnableVertexAttribArray(1));

	if (mode == 2) {
		GCHK(glEnable(GL_BLEND_ADVANCED_COHERENT_KHR));
	}

	GCHK(glDrawArrays(GL_TRIANGLES, 0, 3));

	if (mode == 1) {
		PFNGLBLENDBARRIERKHRPROC *glBlendBarrier = eglGetProcAddress("glBlendBarrier");
		GCHK(glBlendBarrier());
	}

	GCHK(glDrawArrays(GL_TRIANGLES, 0, 3));

	ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	TEST_START();
	TEST(test_advanced_blend(0, 0));
	TEST(test_advanced_blend(2, GL_MULTIPLY_KHR));
	TEST(test_advanced_blend(1, GL_MULTIPLY_KHR));
	TEST(test_advanced_blend(0, GL_MULTIPLY_KHR));
	TEST(test_advanced_blend(0, GL_SCREEN_KHR));
	TEST(test_advanced_blend(0, GL_OVERLAY_KHR));
	TEST(test_advanced_blend(0, GL_DARKEN_KHR));
	TEST(test_advanced_blend(0, GL_LIGHTEN_KHR));
	TEST(test_advanced_blend(0, GL_COLORDODGE_KHR));
	TEST(test_advanced_blend(0, GL_COLORBURN_KHR));
	TEST(test_advanced_blend(0, GL_HARDLIGHT_KHR));
	TEST(test_advanced_blend(0, GL_SOFTLIGHT_KHR));
	TEST(test_advanced_blend(0, GL_DIFFERENCE_KHR));
	TEST(test_advanced_blend(0, GL_EXCLUSION_KHR));
	TEST_END();
	return 0;
}

