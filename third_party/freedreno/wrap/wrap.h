/*
 * Copyright © 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WRAP_H_
#define WRAP_H_

#ifndef BIONIC
#  include <dlfcn.h>
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#define __user
#include "kgsl_drm.h"
#include "msm_kgsl.h"
#include "android_pmem.h"
#include "z180.h"
#include "list.h"
#include "redump.h"

#if 0 /* uncomment for printf in logcat */
int wrap_printf(const char *format, ...);
#define printf wrap_printf
#define USE_PTHREADS
#endif

// GOOGLE: Don't call printf to print debug message to console.
#define printf(...) do { } while(0)

void * __rd_dlsym_helper(const char *name);

#define PROLOG(func) 					\
	static typeof(func) *orig_##func = NULL;	\
	if (!orig_##func)				\
		orig_##func = __rd_dlsym_helper(#func);	\


unsigned int env2u(const char *name);
int wrap_get_next_fd(int index, int *fd);
struct list *wrap_get_buffers_of_interest(int device_fd);
unsigned int wrap_safe(void);
unsigned int wrap_gpu_id(void);
unsigned int wrap_gpu_id_patchid(void);
unsigned int wrap_chip_id(void);
unsigned int wrap_gmem_size(void);
unsigned int wrap_buf_len_cap(void);
unsigned int wrap_dump_all_bo(void);

void hexdump(const void *data, int size);

// GOOGLE: Need this mutex to make sure only 1 thread can dump bos (dump_bos)
// This is required since we assume RD_GPUADDR is always followed by RD_BUFFER_CONTENTS
// Using PTHREAD_RECURSIVE_MUTEX_INITIALIZER since log_gpuaddr can be called in different places
static pthread_mutex_t bo_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static void log_gpuaddr(int device_fd, uint64_t gpuaddr, uint32_t len)
{
    pthread_mutex_lock(&bo_lock);
    uint32_t sect[3] = {
        /* upper 32b of gpuaddr added after len for backwards compat */
			gpuaddr, len, gpuaddr >> 32,
    };
    rd_write_section(device_fd, RD_GPUADDR, sect, sizeof(sect));
    pthread_mutex_unlock(&bo_lock);
}

static void log_cmdaddr(int device_fd, uint64_t gpuaddr, uint32_t sizedwords)
{
	uint32_t sect[3] = {
			/* upper 32b of gpuaddr added after len for backwards compat */
			gpuaddr, sizedwords, gpuaddr >> 32,
	};
	rd_write_section(device_fd, RD_CMDSTREAM_ADDR, sect, sizeof(sect));
}

#if 0
#ifdef USE_PTHREADS
typedef struct
{
    int volatile value;
} pthread_mutex_t;

#define  __PTHREAD_MUTEX_INIT_VALUE            0
#define  __PTHREAD_RECURSIVE_MUTEX_INIT_VALUE  0x4000
#define  __PTHREAD_ERRORCHECK_MUTEX_INIT_VALUE 0x8000

#define  PTHREAD_MUTEX_INITIALIZER             {__PTHREAD_MUTEX_INIT_VALUE}
#define  PTHREAD_RECURSIVE_MUTEX_INITIALIZER   {__PTHREAD_RECURSIVE_MUTEX_INIT_VALUE}
#define  PTHREAD_ERRORCHECK_MUTEX_INITIALIZER  {__PTHREAD_ERRORCHECK_MUTEX_INIT_VALUE}

enum {
	PTHREAD_MUTEX_NORMAL = 0,
	PTHREAD_MUTEX_RECURSIVE = 1,
	PTHREAD_MUTEX_ERRORCHECK = 2,

	PTHREAD_MUTEX_ERRORCHECK_NP = PTHREAD_MUTEX_ERRORCHECK,
	PTHREAD_MUTEX_RECURSIVE_NP  = PTHREAD_MUTEX_RECURSIVE,

	PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
};

int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
#if 0 /* MISSING FROM BIONIC */
int pthread_mutex_timedlock(pthread_mutex_t *mutex, struct timespec*  ts);
#endif /* MISSING */

#endif /* USE_PTHREADS */
#else
// GOOGLE: move this include to the top since it is needed for log_gpuaddr()
// #include <pthread.h>
#endif

#endif /* WRAP_H_ */
