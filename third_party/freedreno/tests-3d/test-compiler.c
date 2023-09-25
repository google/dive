/*
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

#include <GLES3/gl32.h>
#include "test-util-3d.h"

#define HAS_GLES31

int openfile(const char *fmt, int i)
{
	static char path[256];
	sprintf(path, fmt, i);
	return open(path, 0);
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define NVERT 3

static const char *attrnames[] = {
		"aFoo", "aPosition", "aPosition0", "aPosition1", "aPosition2",
		"aTexCoord", "aTexCoord0", "in_position", "a_coords", "a_position",
		"a_in0", "a_in1",
};

static const char *ubonames[] = {
		"ubo0", "ubo1", "ubo2",
};

static EGLDisplay display;
static EGLSurface surface;

struct ubodata {
	float a[512][4];
	float b[512][4];
	float c[512][4];
} ubodata;

const char *readfile(int fd)
{
	static char text[64 * 1024];
	int ret = read(fd, text, sizeof(text));
	if (ret < 0) {
		ERROR_MSG("error reading shader: %d", ret);
		exit(-1);
	}
	text[ret] = '\0';
	return strdup(text);
}

static void compile_shader(GLint program, int fd, const char *stage_name, GLenum type)
{
	GLuint shader;
	GLint ret;
	const char *source = readfile(fd);

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

static void *getpix(unsigned npix)
{
	uint32_t *pix = malloc(npix * 4);
	for (unsigned i = 0; i < npix; i++)
		pix[i] = i;
	return pix;
}



static int setup_tex2d(int program, const char *name, int unit, int image)
{
	int handle;
	GLuint tex;

	handle = glGetUniformLocation(program, name);
	if (handle >= 0) {
		DEBUG_MSG("setup %s", name);

		glGenTextures(1, &tex);

		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (image) {
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 200, 200);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 200, 200, GL_RGBA, GL_UNSIGNED_BYTE, getpix(200 * 200 * 4));
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 200, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, getpix(200 * 200 * 4));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4);
#ifndef GL_TEXTURE_LOD_BIAS_EXT
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501
#endif
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS_EXT, 1);
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glUniform1i(handle, unit);

		/* clear any errors, just in case: */
		while (glGetError() != GL_NO_ERROR) {}

#ifdef HAS_GLES31
		if (image)
			GCHK(glBindImageTexture(unit, tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8));
#endif

		unit++;
	}

	return unit;
}

static int setup_tex3d(int program, const char *name, int unit, int image)
{
	int handle;
	GLuint tex;

	handle = glGetUniformLocation(program, name);
	if (handle >= 0) {
		DEBUG_MSG("setup %s", name);

		glGenTextures(1, &tex);

		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_3D, tex);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, 32, 32, 32);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 32, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, getpix(32 * 32 * 32 * 4));

		glUniform1i(handle, unit);

		/* clear any errors, just in case: */
		while (glGetError() != GL_NO_ERROR) {}

#ifdef HAS_GLES31
		if (image)
			GCHK(glBindImageTexture(unit, tex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8));
#endif

		unit++;
	}

	return unit;
}

static int setup_texcube(int program, const char *name, int unit)
{
	int handle;
	GLuint tex;

	handle = glGetUniformLocation(program, name);
	if (handle >= 0) {
		DEBUG_MSG("setup %s", name);

		glGenTextures(1, &tex);

		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA32F, 200, 200, 0, GL_RGBA, GL_FLOAT, getpix(200 * 200 * 4));
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA32F, 200, 200, 0, GL_RGBA, GL_FLOAT, getpix(200 * 200 * 4));
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA32F, 200, 200, 0, GL_RGBA, GL_FLOAT, getpix(200 * 200 * 4));
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA32F, 200, 200, 0, GL_RGBA, GL_FLOAT, getpix(200 * 200 * 4));
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA32F, 200, 200, 0, GL_RGBA, GL_FLOAT, getpix(200 * 200 * 4));
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA32F, 200, 200, 0, GL_RGBA, GL_FLOAT, getpix(200 * 200 * 4));

		glUniform1i(handle, unit);

		unit++;
	}

	return unit;
}

static void setup_textures(GLint program)
{
	int unit = 0;

	unit = setup_tex2d(program, "uTexture2D", unit, 0);
	unit = setup_tex2d(program, "uTex2D0",    unit, 0);
	unit = setup_tex2d(program, "uTex2D1",    unit, 0);
	unit = setup_tex2d(program, "uTex2D2",    unit, 0);
	unit = setup_tex2d(program, "uTex2D3",    unit, 0);
	unit = setup_tex2d(program, "uTex2D4",    unit, 0);
	unit = setup_tex2d(program, "uTex2D5",    unit, 0);
	unit = setup_tex3d(program, "uTexture3D", unit, 0);
	unit = setup_tex3d(program, "uTex3D0",    unit, 0);
	unit = setup_tex3d(program, "uTex3D1",    unit, 0);
	unit = setup_texcube(program, "uTexCube0", unit);
	unit = setup_texcube(program, "uTexCube1", unit);
	unit = setup_texcube(program, "uTexCube2", unit);
	unit = setup_texcube(program, "uTexCube3", unit);

	unit = setup_tex2d(program, "uImage2D0",  unit, 1);
	unit = setup_tex2d(program, "uImage2D1",  unit, 1);
	unit = setup_tex3d(program, "uImage3D0",  unit, 1);
	unit = setup_tex3d(program, "uImage3D1",  unit, 1);

	// TODO other texture types..
}

static void setup_ssbo(GLint program, const char *name)
{
#ifdef HAS_GLES31
	GLuint ssbo = 0, block_index;
	static int cnt = 0;

	block_index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
	if (block_index == GL_INVALID_INDEX)
		return;

	DEBUG_MSG("SSBO: %s at %u", name, block_index);

	int sz = 33 * ++cnt;
//	int buf[sz];
	static char buf[0x100000];

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, block_index, ssbo);
#endif
}

static void setup(void);

static unsigned samples;

static int test_compiler(int n)
{
	static GLfloat v[ARRAY_SIZE(attrnames)][NVERT * 4];
	static int nattr = 0;
	GLuint program;
	int vert_fd, frag_fd, gs_fd, tcs_fd, tes_fd, cs_fd;
	int i, ret;

	vert_fd = openfile("shaders/%04d.vs", n);
	tcs_fd = openfile("shaders/%04d.tcs", n);
	tes_fd = openfile("shaders/%04d.tes", n);
	gs_fd = openfile("shaders/%04d.gs", n);
	frag_fd = openfile("shaders/%04d.fs", n);
	cs_fd = openfile("shaders/%04d.cs", n);
	if ((vert_fd < 0) || (frag_fd < 0)) {
		if (cs_fd >= 0) {
			RD_START("compiler-compute", "%d", n);
			setup();
			program = get_compute_program(readfile(cs_fd));
			goto link;
		}
		return -1;
	}

	samples = env2u("MSAA");

	if (samples) {
		RD_START("compiler-msaa", "%d (samples=%d)", n, samples);
	} else {
		RD_START("compiler", "%d", n);
	}

	setup();

	program = get_program(readfile(vert_fd), readfile(frag_fd));

	if (tcs_fd >= 0)
		compile_shader(program, tcs_fd, "tcs",  0x8E88/*GL_TESS_CONTROL_SHADER*/);
	if (tes_fd >= 0)
		compile_shader(program, tes_fd, "tes", 0x8E87/*GL_TESS_EVALUATION_SHADER*/);
	if (gs_fd >= 0)
		compile_shader(program, gs_fd, "geom", 0x8DD9/*GL_GEOMETRY_SHADER*/);

	for (i = 0; i < ARRAY_SIZE(attrnames); i++) {
		glBindAttribLocation(program, i, attrnames[i]);
		if (glGetError() == GL_NO_ERROR) {
			printf("use attribute: %s\n", attrnames[i]);
			nattr++;
		}
		/* clear any errors, just in case: */
		while (glGetError() != GL_NO_ERROR) {}
	}

link:

	link_program(program);

	GCHK(glFlush());

	for (i = 0; i < nattr; i++) {
		GCHK(glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, v[i]));
		GCHK(glEnableVertexAttribArray(i));
	}

	for (i = 0; i < ARRAY_SIZE(ubonames); i++) {
		GLuint idx = glGetUniformBlockIndex(program, ubonames[i]);
		GLuint ubo;

		if (idx == GL_INVALID_INDEX)
			continue;

		GCHK(glGenBuffers(1, &ubo));
		GCHK(glBindBuffer(GL_UNIFORM_BUFFER, ubo));
		GCHK(glBufferData(GL_UNIFORM_BUFFER, sizeof(ubodata), &ubodata, GL_DYNAMIC_DRAW));
		GCHK(glBindBuffer(GL_UNIFORM_BUFFER, 0));

		GCHK(glBindBufferBase(GL_UNIFORM_BUFFER, i, ubo));
		GCHK(glUniformBlockBinding(program, idx, i));
	}

	setup_textures(program);
	setup_ssbo(program, "buffer_In");
	setup_ssbo(program, "buffer_In2");
	setup_ssbo(program, "buffer_In3");
	setup_ssbo(program, "buffer_Out");
	setup_ssbo(program, "buffer_Out2");
	setup_ssbo(program, "buffer_Out3");

	/* clear any errors, just in case: */
	while (glGetError() != GL_NO_ERROR) {}

	if (tes_fd >= 0) {
		GCHK(glDrawArrays(0x000E/*GL_PATCHES*/, 0, NVERT));
	} else if (cs_fd >= 0) {
		PFNGLDISPATCHCOMPUTEPROC glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)eglGetProcAddress("glDispatchCompute");
		GCHK(glDispatchCompute(1, 2, 3));
	} else {
		GCHK(glDrawArrays(GL_POINTS, 0, NVERT));
	}

	ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

	ECHK(eglDestroySurface(display, surface));
	ECHK(eglTerminate(display));

	RD_END();

	return 0;
}

static void setup(void)
{
	GLint width, height;
	EGLint pbuffer_attribute_list[] = {
		EGL_WIDTH, 256,
		EGL_HEIGHT, 256,
		EGL_LARGEST_PBUFFER, EGL_TRUE,
		EGL_NONE
	};
	const EGLint config_attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_DEPTH_SIZE, 8,
		EGL_SAMPLES, samples,
		EGL_NONE
	};
	const EGLint context_attribute_list[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	EGLConfig config;
	EGLint num_config;
	EGLContext context;

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));
	ECHK(surface = eglCreatePbufferSurface(display, config, pbuffer_attribute_list));

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("PBuffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));
	GCHK(glFlush());
}

int main(int argc, char *argv[])
{
	TEST_START();

	if (__test == -1) {
		int i;
		for (i = 0; ; i++) {
			int ret = 0;
			if (test_compiler(i)) {
				break;
			}
		}
	} else {
		if (test_compiler(__test)) {
			exit(42);
		}
	}

	return 0;
}

