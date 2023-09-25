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

#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>

#include <GLES3/gl32.h>
#include "test-util-3d.h"

#define HAS_GLES31

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static EGLDisplay display;
static EGLSurface surface;


struct shader {
	const char *text;
	int length;
	int type;
};

struct binding_list {
	char *name;
	GLint index;
	struct binding_list *prev;
};

#define SKIP_SPACES(str) while (*(str) == ' ') str++

/* based on get_shaders() from shader-db: */
static struct shader *
get_shaders(const char *text, size_t text_size, unsigned *num_shaders,
		const char *shader_name, struct binding_list *binding)
{
	static const char *req = "[require]";
	static const char *gl_req = "\nGL >= ";
	static const char *glsl_req = "\nGLSL >= ";
	static const char *glsl_es_req = "\nGLSL ES >= ";
	static const char *sso_req = "\nSSO ENABLED";
	static const char *binding_req = "\nBindAttribLoc";
	static const char *gs = "geometry shader]\n";
	static const char *fs = "fragment ";
	static const char *vs = "vertex ";
	static const char *tcs = "tessellation control shader]\n";
	static const char *tes = "tessellation evaluation shader]\n";
	static const char *cs = "compute shader]\n";
	static const char *shder = "shader]\n";
	static const char *test = "test]\n";
	const char *end_text = text + text_size;

	/* Find the [require] block and parse it first. */
	text = memmem(text, end_text - text, req, strlen(req)) + strlen(req);

	/* Skip the GL >= x.y line if present. */
	if (memcmp(text, gl_req, strlen(gl_req)) == 0) {
		text += strlen(gl_req) + 3; /* for x.y */
	}

	if (memcmp(text, glsl_req, strlen(glsl_req)) == 0) {
		text += strlen(glsl_req);

		long major = strtol(text, (char **)&text, 10);
		long minor = strtol(text + 1, (char **)&text, 10);
		long version = major * 100 + minor;

		/* android blob does not support desktop GL.. */
		fprintf(stderr, "SKIP: %s requires GLSL %ld\n",
				shader_name, version);
		return NULL;
	} else if (memcmp(text, glsl_es_req, strlen(glsl_es_req)) == 0) {
		text += strlen(glsl_es_req);
		long major = strtol(text, (char **)&text, 10);
		long minor = strtol(text + 1, (char **)&text, 10);
		long version = major * 100 + minor;

		fprintf(stderr, "%s requires GLSL ES %ld\n",
				shader_name, version);
	} else {
		fprintf(stderr, "ERROR: Unexpected token in %s\n", shader_name);
		return NULL;
	}

	const char *extension_text = text;
	const char *extension_string = (char *)glGetString(GL_EXTENSIONS);
	int extension_string_len = strlen(extension_string);

	while ((extension_text = memmem(extension_text, end_text - extension_text,
			"\nGL_", strlen("\nGL_"))) != NULL) {
		extension_text += 1;
		const char *newline = memchr(extension_text, '\n',
				end_text - extension_text);

		if (memmem(extension_string, extension_string_len,
				extension_text, newline - extension_text) == NULL) {
			fprintf(stderr, "SKIP: %s requires unavailable extension %.*s\n",
					shader_name, (int)(newline - extension_text), extension_text);
			return NULL;
		}
	}

	/* process binding */
	struct binding_list *binding_prev = binding = NULL;
	const char *pre_binding_text = text;

	/* binding = NULL if there's no binding required */
	binding = NULL;
	while ((pre_binding_text = memmem(pre_binding_text, end_text - pre_binding_text,
			binding_req, strlen(binding_req))) != NULL) {
		pre_binding_text += strlen(binding_req);

		const char *newline = memchr(pre_binding_text, '\n', end_text - pre_binding_text);

		SKIP_SPACES(pre_binding_text);

		char *endword = memchr(pre_binding_text, ' ', newline - pre_binding_text);

		/* if there's no more space in the same line */
		if (!endword) {
			fprintf(stderr, "SKIP: can't find attr index for this binding\n");
			continue;
		}

		char *binding_name = calloc(1, endword - pre_binding_text + 1);

		strncpy(binding_name, pre_binding_text, endword - pre_binding_text);

		pre_binding_text = endword;

		SKIP_SPACES(pre_binding_text);
		if (*pre_binding_text == '\n') {
			fprintf(stderr, "SKIP: can't find attr variable name for this binding\n");
			continue;
		}

		endword = memchr(pre_binding_text, ' ', newline - pre_binding_text);

		if (!endword)
			endword = (char *)newline;

		char *index_string = calloc(1, endword - pre_binding_text + 1);
		strncpy(index_string, pre_binding_text, endword - pre_binding_text);

		struct binding_list *binding_new = malloc(sizeof(struct binding_list));

		binding_new->index = strtol(index_string, NULL, 10);
		binding_new->name = binding_name;
		binding_new->prev = binding_prev;
		binding = binding_prev = binding_new;

		free(index_string);

		fprintf(stdout,
				"LOG: glBindAttribLocation(prog, %d, \"%s\") will be executed before linking\n",
				binding_new->index, binding_new->name);
	}

	/* Find the shaders. */
	unsigned shader_size = 3;
	struct shader *shader = malloc(shader_size * sizeof(struct shader));
	unsigned i = 0;
	while ((text = memmem(text, end_text - text, "\n[", strlen("\n["))) != NULL) {
		const char *save_text = text;
		text += strlen("\n[");

		if (shader_size == i)
			shader = realloc(shader, ++shader_size * sizeof(struct shader));

		if (memcmp(text, fs, strlen(fs)) == 0) {
			text += strlen(fs);
			if (memcmp(text, shder, strlen(shder)) == 0) {
				shader[i].type = GL_FRAGMENT_SHADER;
				text += strlen(shder);
			}
			shader[i].text = text;
		} else if (memcmp(text, vs, strlen(vs)) == 0) {
			text += strlen(vs);
			if (memcmp(text, shder, strlen(shder)) == 0) {
				shader[i].type = GL_VERTEX_SHADER;
				text += strlen(shder);
			}
			shader[i].text = text;
		} else if (memcmp(text, gs, strlen(gs)) == 0) {
			text += strlen(gs);
			shader[i].type = GL_GEOMETRY_SHADER;
			shader[i].text = text;
		} else if (memcmp(text, tcs, strlen(tcs)) == 0) {
			text += strlen(tcs);
			shader[i].type = GL_TESS_CONTROL_SHADER;
			shader[i].text = text;
		} else if (memcmp(text, tes, strlen(tes)) == 0) {
			text += strlen(tes);
			shader[i].type = GL_TESS_EVALUATION_SHADER;
			shader[i].text = text;
		} else if (memcmp(text, cs, strlen(cs)) == 0) {
			text += strlen(cs);
			shader[i].type = GL_COMPUTE_SHADER;
			shader[i].text = text;
		} else if (memcmp(text, test, strlen(test)) == 0) {
			shader[i - 1].length = save_text + 1 - shader[i - 1].text;
			goto out;
		} else {
			fprintf(stderr, "ERROR: Unexpected token in %s\n", shader_name);
			free(shader);
			return NULL;
		}

		if (i != 0)
			shader[i - 1].length = save_text + 1 - shader[i - 1].text;
		i++;
	}

	shader[i - 1].length = end_text - shader[i - 1].text;

	out:
	*num_shaders = i;
	return shader;
}

#define VTX_CNT (3*123)

static void setup(void);


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
		GLenum fmt, ifmt, type;

		if (strstr(name, "Shadow")) {
			fmt = GL_DEPTH_COMPONENT;
			ifmt = GL_DEPTH_COMPONENT32F;
			type = GL_FLOAT;
		} else {
			fmt = GL_RGBA;
			ifmt = GL_RGBA8;
			type = GL_UNSIGNED_BYTE;
		}

		DEBUG_MSG("setup %s (%s,%s,%s)", name, formatname(fmt), formatname(ifmt), typename(type));
/*
lockup:
dEQP-GLES31.functional.atomic_counter.get_inc_dec.8_counters_1_call_1_thread


good:
dEQP-GLES31.functional.image_load_store.2d.atomic.add_r32i_result


bad:
dEQP-GLES31.functional.image_load_store.2d.atomic.add_r32i_return_value
dEQP-GLES31.functional.image_load_store.2d_array.atomic.add_r32i_result


imageStore(u_returnValues, ivec2(gx, gy), ivec4(
imageAtomicAdd(u_results, ivec2(gx % 64, gy), (gx*gx + gy*gy + gz*gz))));

imageAtomicAdd(u_results, ivec2(gx % 64, gy), (gx*gx + gy*gy + gz*gz));
 */
		glGenTextures(1, &tex);

		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (image) {
			glTexStorage2D(GL_TEXTURE_2D, 1, ifmt, 200, 200);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 200, 200, fmt, type, getpix(200 * 200 * 4));
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, ifmt, 200, 200, 0, fmt, type, getpix(200 * 200 * 4));
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
			GCHK(glBindImageTexture(unit, tex, 0, GL_FALSE, 0, GL_READ_WRITE, ifmt));
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
		GLenum fmt, ifmt, type;

		if (strstr(name, "Shadow")) {
			fmt = GL_DEPTH_COMPONENT;
			ifmt = GL_DEPTH_COMPONENT32F;
			type = GL_FLOAT;
		} else {
			fmt = GL_RGBA;
			ifmt = GL_RGBA8;
			type = GL_UNSIGNED_BYTE;
		}

		DEBUG_MSG("setup %s (%s,%s,%s)", name, formatname(fmt), formatname(ifmt), typename(type));

		glGenTextures(1, &tex);

		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_3D, tex);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexStorage3D(GL_TEXTURE_3D, 1, ifmt, 32, 32, 32);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 32, 32, 32, fmt, type, getpix(32 * 32 * 32 * 4));

		glUniform1i(handle, unit);

		/* clear any errors, just in case: */
		while (glGetError() != GL_NO_ERROR) {}

#ifdef HAS_GLES31
		if (image)
			GCHK(glBindImageTexture(unit, tex, 0, GL_TRUE, 0, GL_READ_WRITE, ifmt));
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
	unit = setup_tex2d(program, "uTex2ShadowD0", unit, 0);
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


static int test_compiler(const char *path)
{
	static int nattr = 0;
	int fd;
	int i, ret;

	char *name = getenv("TESTNAME");
	if (!name)
		name = "shader-runner";

	RD_START(name, "%s", path);

	fd = open(path, 0);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	setup();

	struct stat statbuf;

	ret = fstat(fd, &statbuf);
	if (ret < 0) {
		ERROR_MSG("error reading shader: %d", ret);
		exit(-1);
	}

	const char *text =
		mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	struct binding_list *binding = NULL;
	unsigned num_shaders;

	struct shader *shader =
		get_shaders(text, statbuf.st_size, &num_shaders, path, &binding);

	GLuint prog = glCreateProgram();
	GLint param;

	for (unsigned i = 0; i < num_shaders; i++) {
		GLuint s = glCreateShader(shader[i].type);

		DEBUG_MSG("%04x shader:\n%s", shader[i].type, shader[i].text);

		glShaderSource(s, 1, &shader[i].text, &shader[i].length);
		glCompileShader(s);

		GCHK(glGetShaderiv(s, GL_COMPILE_STATUS, &param));
		if (!param) {
			GLchar log[4096];
			GLsizei length;
			GCHK(glGetShaderInfoLog(s, 4096, &length, log));

			fprintf(stderr, "ERROR: %s failed to compile:\n%s\n",
					path, log);
		}
		GCHK(glAttachShader(prog, s));
		GCHK(glDeleteShader(s));
	}

	/* takes care of pre-bindings */
	while (binding != NULL) {
		struct binding_list *prev = binding->prev;
		GCHK(glBindAttribLocation(prog, binding->index, binding->name));
		free(binding->name);
		free(binding);
		binding = prev;
	}

	link_program(prog);

	setup_textures(prog);

	/* clear any errors, just in case: */
	while (glGetError() != GL_NO_ERROR) {}


	GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, VTX_CNT));

	readback();

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
	test_compiler(argv[1]);

	return 0;
}

