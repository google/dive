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

static const char *shader_source_fmt =
	"#version 310 es                                                   \n"
	"precision highp float;                                            \n"
	"precision highp int;                                              \n"
	"precision highp image2D;                                          \n"
	"                                                                  \n"
	"layout(local_size_x=%d, local_size_y=%d, local_size_z=%d) in;     \n"
	"                                                                  \n"
	"layout(binding = 0) buffer buffer_In {                            \n"
	"    uvec3 In[];                                                   \n"
	"};                                                                \n"
	"                                                                  \n"
	"layout(binding = 1) buffer buffer_Out {                           \n"
	"    uvec3 Out[];                                                  \n"
	"};                                                                \n"
	"                                                                  \n"
	"layout(rgba8) readonly uniform image2D src0;                      \n"
	"layout(rgba8) readonly uniform image2D src1;                      \n"
	"uniform sampler2D uTex2D0;                                        \n"
	"                                                                  \n"
	"uniform uint max;                                                 \n"
	"uniform mat3 uMat[16];                                            \n"
	"uniform mat3 uMat0;                                               \n"
	"uniform mat3 uMat1;                                               \n"
	"uniform mat3 uMat2;                                               \n"
	"uniform mat3 uMat3;                                               \n"
	"                                                                  \n"
	"void main(void) {                                                 \n"
	"    uint index = gl_LocalInvocationIndex;                         \n"
	"    if (index > max)                                              \n"
	"        return;                                                   \n"
	"    Out[index] = In[index] + %s;                                  \n"
	"}                                                                 \n"
	"\n";

static const char * shader_source(int x, int y, int z, const char *id)
{
	char *source;
	asprintf(&source, shader_source_fmt, x, y, z, id);
	return source;
}

#ifndef GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS 0x90D7
#endif
#ifndef GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS 0x90D8
#endif
#ifndef GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS 0x90D9
#endif

#ifndef GL_MAX_GEOMETRY_IMAGE_UNIFORMS
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS    0x90CD
#endif
#ifndef GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS 0x90CB
#endif
#ifndef GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS 0x90CC
#endif

static const struct {
	const char *str;
	GLenum glenum;
#define ENUM(x) { #x, x }
} enums[] = {
	ENUM(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS),
	ENUM(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS),
	ENUM(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT),
	ENUM(GL_MAX_SHADER_STORAGE_BLOCK_SIZE),

	ENUM(GL_MAX_VERTEX_IMAGE_UNIFORMS),
	ENUM(GL_MAX_GEOMETRY_IMAGE_UNIFORMS),
	ENUM(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS),
	ENUM(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS),
	ENUM(GL_MAX_FRAGMENT_IMAGE_UNIFORMS),
	ENUM(GL_MAX_COMPUTE_IMAGE_UNIFORMS),
	ENUM(GL_MAX_COMBINED_IMAGE_UNIFORMS),

	ENUM(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS),
	ENUM(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE),
};

static void test_compute(int x, int y, int z, int ngroup, const char *id)
{
	RD_START("compute", "x=%d, y=%d, z=%d, ngroup=%d, id=%s", x, y, z, ngroup, id);

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));
	ECHK(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context));

	DEBUG_MSG("EGL Version %s", eglQueryString(display, EGL_VERSION));
	DEBUG_MSG("EGL Vendor %s", eglQueryString(display, EGL_VENDOR));
	DEBUG_MSG("EGL Extensions %s", eglQueryString(display, EGL_EXTENSIONS));
	DEBUG_MSG("GL Version %s", glGetString(GL_VERSION));
	DEBUG_MSG("GL extensions: %s", glGetString(GL_EXTENSIONS));

	/* print some compute limits (not strictly necessary) */
	GLint work_group_count[3] = {0};
	for (unsigned i = 0; i < 3; i++)
		GCHK(glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &work_group_count[i]));
	DEBUG_MSG("GL_MAX_COMPUTE_WORK_GROUP_COUNT: %d, %d, %d",
			work_group_count[0],
			work_group_count[1],
			work_group_count[2]);

	GLint work_group_size[3] = {0};
	for (unsigned i = 0; i < 3; i++)
		GCHK(glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &work_group_size[i]));
	DEBUG_MSG("GL_MAX_COMPUTE_WORK_GROUP_SIZE: %d, %d, %d",
			work_group_size[0],
			work_group_size[1],
			work_group_size[2]);

	GLint val, i;
	for (i = 0; i < ARRAY_SIZE(enums); i++) {
		glGetIntegerv(enums[i].glenum, &val);
		DEBUG_MSG("%s: %d", enums[i].str, val);
	}

	/* setup a compute shader */

	GLuint program = get_compute_program(shader_source(x, y, z, id));

	link_program(program);

	/* dispatch computation */
	if (ngroup < 0) {
		/* indirect dispatch */
		int cnt = -ngroup;
		PFNGLDISPATCHCOMPUTEINDIRECTPROC glDispatchComputeIndirect =
			(PFNGLDISPATCHCOMPUTEINDIRECTPROC)eglGetProcAddress("glDispatchComputeIndirect");
		struct {
			uint32_t num_groups_x;
			uint32_t num_groups_y;
			uint32_t num_groups_z;
		} data[0x1000 * cnt];
		GLuint buffer;

		for (i = 0; i < 3 * cnt; i++) {
			data[i].num_groups_x = i * 1;
			data[i].num_groups_y = i * 2;
			data[i].num_groups_z = i * 3;
		}

		GCHK(glGenBuffers(1, &buffer));
		GCHK(glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer));
		GCHK(glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(data), data, GL_STATIC_DRAW));

		GCHK(glDispatchComputeIndirect((GLintptr)(cnt * 4)));
	} else {
		/* direct dispatch */
		PFNGLDISPATCHCOMPUTEPROC glDispatchCompute =
			(PFNGLDISPATCHCOMPUTEPROC)eglGetProcAddress("glDispatchCompute");
		GCHK(glDispatchCompute(1 * ngroup, 2 * ngroup, 3 * ngroup));
	}

	DEBUG_MSG("Compute shader dispatched and finished successfully\n");

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	TEST_START();
	TEST(test_compute(   4,    2,   1, 1, "gl_NumWorkGroups + uvec3(uMat3 * (uMat[gl_GlobalInvocationID.x] * (uMat2 * (uMat1 * (uMat0 * texture(uTex2D0, vec2(gl_GlobalInvocationID.xy)).xyz)))))"));
	TEST(test_compute(   4,    2,   1, 1, "gl_NumWorkGroups + uvec3(100.0 * texture(uTex2D0, vec2(gl_GlobalInvocationID.xy)).xyz)"));
	TEST(test_compute(   4,    2,   1, 1, "gl_NumWorkGroups + uvec3(imageLoad(src0, ivec2(gl_GlobalInvocationID.xy)).xyz)"));
	TEST(test_compute(   4,    2,   1, 1, "gl_NumWorkGroups + uvec3(imageLoad(src0, ivec2(gl_GlobalInvocationID.xy)).xyz) + uvec3(imageLoad(src1, ivec2(gl_GlobalInvocationID.xy)).xyz)"));
	TEST(test_compute(   4,    2,   1, 1, "uvec3(imageSize(src1).xyy)"));
	TEST(test_compute(   4,    2,   1, 1, "uvec3(In.length())"));
	TEST(test_compute(   4,    2,   1, 1, "gl_NumWorkGroups"));
	TEST(test_compute(   4,    2,   1, 1, "gl_WorkGroupID"));
	TEST(test_compute(   4,    2,   1, 1, "gl_LocalInvocationID"));
	TEST(test_compute(   4,    2,   1, 1, "gl_GlobalInvocationID"));
	TEST(test_compute(   4,    2,   1, 1, "gl_LocalInvocationIndex"));
	TEST(test_compute(   2,    6,   2, 1, "gl_WorkGroupID"));
	TEST(test_compute( 256,  256,  64,  1, "gl_NumWorkGroups"));
	TEST(test_compute( 256,  256,  64,  7, "gl_NumWorkGroups"));
	TEST(test_compute( 256,  256,  64, -1, "gl_NumWorkGroups"));
	TEST(test_compute( 256,  256,  64, -7, "gl_NumWorkGroups"));
	TEST_END();

	return 0;
}

