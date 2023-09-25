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

#include <GLES3/gl3.h>
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
static const char *vertex_shader_source =
		"#version 300 es              \n"
		"in vec4 in_position;         \n"
		"in vec2 in_TexCoord;         \n"
		"\n"
		"out vec3 vTexCoord;          \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_Position = in_position;\n"
		"    vTexCoord = in_TexCoord.xyy; \n"
		"}                            \n";

static const char *fragment_shader_source =
		"#version 300 es              \n"
		"precision mediump float;     \n"
		"                             \n"
		"uniform sampler2DArray uTexture;  \n"
		"in vec3 vTexCoord;           \n"
		"out vec4 gl_FragColor;       \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = texture(uTexture, vTexCoord);\n"
		"}                            \n";


struct dim {
	int w, h, d;
};

struct fmt {
	GLenum ifmt;
	GLenum fmt;
	GLenum type;
};

static int fmt2components(GLenum fmt)
{
	// TODO more formats
	switch (fmt) {
	case GL_RED_INTEGER:
		return 1;
	case GL_RG_INTEGER:
		return 2;
	case GL_RGB_INTEGER:
		return 3;
	case GL_RGBA_INTEGER:
		return 4;
	default:
		exit(-1);
	}
}

static int fmt2bpp(GLenum type)
{
	// TODO more types
	switch (type) {
	case GL_UNSIGNED_BYTE:
		return 1;
	default:
		exit(-1);
	}
}

static void *pixels(unsigned char val, int sz)
{
	void *p = malloc(sz);
	memset(p, val, sz);
	return p;
}

static unsigned
minify(unsigned value, unsigned levels)
{
	value >>= levels;
	if (value == 0)
		return 1;
	return value;
}

static unsigned
levels2d(int w, int h)
{
	for	(int level = 0; ; level++) {
		w = minify(w, 1);
		h = minify(h, 1);

		if ((w == 1) && (h == 1))
			return level;
	}

	return ~0;
}

static void upload_tex(const struct dim *d, const struct fmt *f)
{
	int bpc = fmt2components(f->fmt) * fmt2bpp(f->type);
	int w = d->w;
	int h = d->h;

	GCHK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, levels2d(d->w, d->h), f->ifmt, d->w, d->h, d->d));

	for (int level = 0, c = 0; ; level++) {
		for (int z = 0; z < d->d; z++) {
			char *ptr = pixels(c++, w * h * bpc);

			/* if texture is big enough to fit a signature in first row,
			 * then do so.. tools can look for this sig and try to parse
			 * the texture layout:
			 */
#define SIG "td2awwwwhhhhdddd"   /* texture 2d array/(w)idth/(h)eight/(d)epth */
			if ((level == 0) && (z == 0) && ((w * bpc) > (strlen(SIG) + 1))) {
				char sig[64] = SIG;
				sprintf(sig, "t2da%04u%04u%04u", d->w, d->h, d->d);
				/* without null terminator.. we want at least one byte of 'c': */
				memcpy(ptr, sig, strlen(sig));
			}

			GCHK(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level,
					0, 0, z, w, h, 1,
					f->fmt, f->type, ptr));

			free(ptr);
		}

		w = minify(w, 1);
		h = minify(h, 1);

		if ((w == 1) && (h == 1))
			break;
	}
}

static void test(const struct dim *d, const struct fmt *f)
{
	GLint width, height;
	GLint modelviewmatrix_handle, modelviewprojectionmatrix_handle, normalmatrix_handle;
	GLuint texturename = 0, texture_handle;
	GLfloat vVertices[] = {
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

	RD_START("layout-2d-array", "texwidth=%d, texheight=%d, texdepth=%d, iformat=%s, format=%s, type=%s",
			d->w, d->h, d->d, formatname(f->ifmt), formatname(f->fmt), typename(f->type));

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

	program = get_program(vertex_shader_source, fragment_shader_source);

	GCHK(glBindAttribLocation(program, 0, "in_position"));
	GCHK(glBindAttribLocation(program, 1, "in_TexCoord"));

	link_program(program);

	GCHK(glViewport(0, 0, width, height));


	/* clear the color buffer */
	GCHK(glClearColor(0.5, 0.5, 0.5, 1.0));
	GCHK(glClear(GL_COLOR_BUFFER_BIT));

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices));
	GCHK(glEnableVertexAttribArray(0));

	GCHK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vTexCoords));
	GCHK(glEnableVertexAttribArray(1));

	GCHK(glActiveTexture(GL_TEXTURE0));
	GCHK(glGenTextures(1, &texturename));
	GCHK(glBindTexture(GL_TEXTURE_2D_ARRAY, texturename));
	upload_tex(d, f);

	/* Note: cube turned black until these were defined. */
	GCHK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GCHK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GCHK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GCHK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GCHK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT));

	GCHK(texture_handle = glGetUniformLocation(program, "uTexture"));
	GCHK(glUniform1i(texture_handle, 0)); /* '0' refers to texture unit 0. */

	GCHK(glEnable(GL_CULL_FACE));

	GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

	ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	static const struct dim s[] = {
			{  17,   4,   2 },
			{  64,   4,   8 },
			{ 128,   4,   2 },
			{ 333, 222,   3 },
	};
	static const struct fmt fmts[] = {
			/* cut-down list from test-tex-fbo.c.. for now only do UNSIGNED_BYTE */
			{ GL_R8UI,          GL_RED_INTEGER,   GL_UNSIGNED_BYTE },
			{ GL_RG8UI,         GL_RG_INTEGER,    GL_UNSIGNED_BYTE },
			{ GL_RGB8UI,        GL_RGB_INTEGER,   GL_UNSIGNED_BYTE },
			{ GL_RGBA8UI,       GL_RGBA_INTEGER,  GL_UNSIGNED_BYTE },
	};
	int i, j;

	TEST_START();

	for (i = 0; i < ARRAY_SIZE(s); i++) {
		for (j = 0; j < ARRAY_SIZE(fmts); j++) {
			TEST(test(&s[i], &fmts[j]));
		}
	}

	TEST_END();

	return 0;
}

