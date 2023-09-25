/*
 * Copyright (c) 2014 Rob Clark <robdclark@gmail.com>
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

#include <CL/opencl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "test-util-common.h"

/* Semi-generic kernel-runner:  The parameters must be immed or __global
 * pointers.  Kernel name must be "simple"
 */

#define CCHK(x) do { \
		int __err; \
		DEBUG_MSG(">>> %s", #x); \
		RD_WRITE_SECTION(RD_CMD, #x, strlen(#x)); \
		__err = x; \
		if (__err != CL_SUCCESS) { \
			ERROR_MSG("<<< %s: failed: %d", #x, __err); \
			exit(-1); \
		} \
		DEBUG_MSG("<<< %s: succeeded", #x); \
	} while (0)

static char buffer[4096 * 4096 * 4];


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


static void run_test(const char *filename)
{
	unsigned int num_platforms;
	size_t len;

	cl_platform_id platform;
	cl_device_id device_id;             // compute device id
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_kernel kernel;                   // compute kernel
	cl_uint i, num_params;
	cl_ulong local_mem_size;

	size_t global[3] = { 64, 64, 64 };
	size_t local[3]  = { 16, 16, 16 };

	const char *kernel_src = readfile(open(filename, 0));

	RD_START("kernel", "%s", filename);
	DEBUG_MSG("shader:\n%s\n", kernel_src);

	CCHK(clGetPlatformIDs(1, &platform, &num_platforms));
	CCHK(clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, sizeof(buffer), buffer, &len));
	DEBUG_MSG("extensions=%s\n", buffer);

	CCHK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL));

	CCHK(clGetDeviceInfo(device_id, CL_DEVICE_VERSION, sizeof(buffer), buffer, &len));
	DEBUG_MSG("version=%s\n", buffer);
	CCHK(clGetDeviceInfo(device_id, CL_DEVICE_EXTENSIONS, sizeof(buffer), buffer, &len));
	DEBUG_MSG("device extensions=%s\n", buffer);

	context = clCreateContext(0, 1, &device_id, NULL, NULL, NULL);
	commands = clCreateCommandQueue(context, device_id, 0, NULL);
	program = clCreateProgramWithSource(context, 1, (const char **) &kernel_src, NULL, NULL);

	CCHK(clBuildProgram(program, 0, NULL, "-cl-mad-enable -DFILTER_SIZE=1 "
			"-DSAMP_MODE=CLK_NORMALIZED_COORDS_FALSE|CLK_ADDRESS_CLAMP_TO_EDGE|CLK_FILTER_NEAREST",
			NULL, NULL));

	kernel = clCreateKernel(program, "simple", NULL);

	CCHK(clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(num_params), &num_params, NULL));
	DEBUG_MSG("num_params=%u\n", num_params);

	for (i = 0; i < num_params; i++) {
		cl_int err;
		cl_mem mem;

		mem = clCreateBuffer(context, CL_MEM_READ_WRITE, 0x1000, NULL, &err);
		CCHK(clSetKernelArg(kernel, i, sizeof(cl_mem), (void*)&mem));
	}

	CCHK(clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_LOCAL_MEM_SIZE,
			sizeof(local_mem_size), &local_mem_size, NULL));

	DEBUG_MSG("local_mem_size=%lu\n", local_mem_size);

	CCHK(clEnqueueNDRangeKernel(commands, kernel, 1, NULL, global, local, 0, NULL, NULL));

	CCHK(clFinish(commands));

	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);

	RD_END();
}

int main(int argc, char **argv)
{
	TEST_START();
	TEST(run_test(argv[1]));
	TEST_END();
}
