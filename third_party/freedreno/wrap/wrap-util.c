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

#include "wrap.h"

// GOOGLE: Include Dive related header
#include "dive-wrap.h"

int IsCapturing();
#define COMPRESS_TRACE
#ifdef COMPRESS_TRACE
#include <zlib.h>
#define LOG_FILE_TYPE gzFile
#define LOG_NULL_FILE NULL
#define LOG_PRI_FILE "p"
#define LOG_OPEN_FILE(file) gzopen((file), "w")
#define LOG_CLOSE_FILE(file) gzclose((file))
#define LOG_WRITE_FILE(file, buf, size) gzwrite((file), (buf), (size))
#define LOG_SYNC_FILE(file)	gzflush((file), Z_FINISH)
#define LOG_ERROR(file, err) gzerror((file), NULL)
#else
#define LOG_FILE_TYPE int
#define LOG_NULL_FILE -1
#define LOG_PRI_FILE "d"
#define LOG_OPEN_FILE(file) open((file), O_WRONLY| O_TRUNC | O_CREAT, 0644)
#define LOG_CLOSE_FILE(file) close((file))
#define LOG_WRITE_FILE(file, buf, size) write((file), (buf), (size))
#define LOG_SYNC_FILE(file) fsync((file))
#define LOG_ERROR(file, err) strerror(err)
#endif

#define MAX_DEVICE_FILES 1024

struct device_file {
	int device_fd;
	LOG_FILE_TYPE log_fd;
	int open_count;
	char file_name[PATH_MAX];
	struct list buffers_of_interest;
};
static struct device_file device_files[MAX_DEVICE_FILES] = {
	[0 ... MAX_DEVICE_FILES-1] = {-1, LOG_NULL_FILE, 0, {0}, {0}}
};
static unsigned int gpu_id;
// GOOGLE: Store the chip ID.
static uint64_t chip_id = 0;

#ifdef USE_PTHREADS
static pthread_mutex_t l = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#endif

static pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;

char *getcwd(char *buf, size_t size);

int __android_log_print(int prio, const char *tag,  const char *fmt, ...);

static char tracebuf[4096], *tracebufp = tracebuf;

static struct device_file *get_file(int device_fd)
{
	if (device_fd == -1)
		return &device_files[0];

	for (int i = 0; i < MAX_DEVICE_FILES; i++) {
		struct device_file *df = &device_files[i];
		if (device_fd == df->device_fd)
			return df;
	}
	return NULL;
}

static struct device_file *add_file(int device_fd)
{
	if (device_fd == -1)
		return &device_files[0];

	for (int i = 0; i < MAX_DEVICE_FILES; i++) {
		struct device_file *df = &device_files[i];
		if (df->device_fd == -1) {
			df->device_fd = device_fd;
			// GOOGLE: Add debug log.
			LOGD("add_file: device_fd %d, log_fd %p, i %d \n", device_fd, df->log_fd, i);
			df->buffers_of_interest = (struct list)LIST_HEAD_INIT(df->buffers_of_interest);
			return df;
		}
	}

	assert(0 && "Tried to add more than MAX_DEVICE_FILES device files");
	return NULL;
}

int wrap_get_next_fd(int index, int *fd) {
	for (int i = index; i < MAX_DEVICE_FILES; i++) {
		struct device_file *df = &device_files[i];
		if(df->device_fd != -1) {
			*fd = df->device_fd;
			return i+1;
		}
	}

	return -1;
}

struct list *wrap_get_buffers_of_interest(int device_fd) {
	struct device_file *df = get_file(device_fd);
	if(df == NULL)
		return NULL;

	return &df->buffers_of_interest;
}

int wrap_printf(const char *format, ...)
{
	int n;
	va_list args;

#ifdef USE_PTHREADS
	pthread_mutex_lock(&l);
#endif

	va_start(args, format);
	n = vsprintf(tracebufp, format, args);
	tracebufp += n;
	va_end(args);

	if (strstr(tracebuf, "\n") || ((tracebufp - tracebuf) > (sizeof(tracebuf)/2))) {
		char *p2, *p = tracebuf;
		while ((p < tracebufp) && (p2 = strstr(p, "\n"))) {
			*p2 = '\0';
			__android_log_print(5, "WRAP", "%s\n", p);
			p = p2 + 1;
		}
		memcpy(tracebuf, p, tracebufp - p);
		tracebufp = tracebuf + (tracebufp - p);
	}

#ifdef USE_PTHREADS
	pthread_mutex_unlock(&l);
#endif

	return n;
}


void rd_start(int device_fd, const char *name, const char *fmt, ...)
{
	char buf[PATH_MAX];
	static int cnt = 0;
	int n = cnt++;
	const char *testnum;
	va_list  args;

	// GOOGLE: Use exsiting device_file if already being added.
	struct device_file *df = get_file(device_fd);
	if(df == NULL)
	{
		LOGD("rd_start add new device file %d\n", device_fd);
		df = add_file(device_fd);
	}

	if(!IsCapturing()) {
		return;
	}
	LOGD("rd_start with device_fd %d\n", device_fd);

	assert(df != NULL);
	if (df->log_fd != LOG_NULL_FILE)
		return;

	if (!name) {
		name = "trace";
	}

	testnum = getenv("TESTNUM");
	if (testnum) {
		n = strtol(testnum, NULL, 0);
		// GOOGLE: add open_count to the filename.
		sprintf(buf, "%s-%04u-%d.rd.inprogress", name, n, df->open_count);
	} else {
		if (device_fd == -1) {
			if (df->open_count == 0)
				sprintf(buf, "/sdcard/Download/%s.rd.inprogress", name);
			else
				sprintf(buf, "/sdcard/Download/%s-%d.rd.inprogress", name, df->open_count);
		}
		else {
			if (df->open_count == 0)
				sprintf(buf, "/sdcard/Download/%s-fd%d.rd.inprogress", name, device_fd);
			else
				sprintf(buf, "/sdcard/Download/%s-fd%d-%d.rd.inprogress", name, device_fd, df->open_count);
		}
	}

	df->log_fd = LOG_OPEN_FILE(buf);
	// GOOGLE: Add debug log.
	LOGD("LOG_OPEN_FILE: device_fd %d, log_fd %p buf %s\n", df->device_fd, df->log_fd, buf);
	if(df->log_fd == LOG_NULL_FILE) {
		LOGD("Failed to LOG_OPEN_FILE (%s)", strerror(errno));
	}
	assert(df->log_fd != LOG_NULL_FILE);
	strcpy(df->file_name, buf);
	df->open_count++;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	rd_write_section(device_fd, RD_TEST, buf, strlen(buf));

	if (gpu_id) {
		/* no guarantee that blob driver will again get devinfo property,
		 * so we could miss the GPU_ID section in the new rd file.. so
		 * just hack around it:
		 */
		rd_write_section(device_fd, RD_GPU_ID, &gpu_id, sizeof(gpu_id));
	}

	// GOOGLE: Write out the chip id.
	if(chip_id) {
		rd_write_section(device_fd, RD_CHIP_ID, &chip_id, sizeof(chip_id));
	}
}

void rd_end(int device_fd)
{
	char new_file_name[PATH_MAX];
	struct device_file *df = get_file(device_fd);
	if (df == NULL)
		return;
	// GOOGLE: Add debug log.
	LOGD("rd_end remove device_fd %d, log_fd %p\n", device_fd, df->log_fd);
	LOG_CLOSE_FILE(df->log_fd);

	/* Rename file from rd_inprogress to rd */
	int file_name_len = strlen(df->file_name);
	strcpy(new_file_name, df->file_name);

	/* set last '.' to be new null terminator */
	new_file_name[file_name_len - 11] = '\0';

	if (rename(df->file_name, new_file_name) == -1) {
		printf("Failed to rename trace file (%s)", strerror(errno));
	}

	df->log_fd = LOG_NULL_FILE;
	df->device_fd = -1;
}

#if 0
volatile int*  __errno( void );
#undef errno
#define errno (*__errno())
#endif

static void rd_write(int device_fd, const void *buf, int sz)
{
	struct device_file *df = get_file(device_fd);
	assert(df != NULL);
	const uint8_t *cbuf = buf;
	while (sz > 0) {
		int ret = LOG_WRITE_FILE(df->log_fd, cbuf, sz);
		if (ret < 0) {
			printf("error: %d (%s)\n", ret, LOG_ERROR(df->log_fd, errno));
			printf("fd=%"LOG_PRI_FILE", buf=%p, sz=%d\n", df->log_fd, buf, sz);
			exit(-1);
		}
		cbuf += ret;
		sz -= ret;
	}
}

void rd_write_section(int device_fd, enum rd_sect_type type, const void *buf, int sz)
{
	uint32_t val = ~0;

	// GOOGLE: cache gup_id and chip_id.
	if (type == RD_GPU_ID) {
		gpu_id = *(unsigned int *)buf;
	}
	if (type == RD_CHIP_ID) {
		chip_id = *(uint64_t *)buf;
	}


	struct device_file *df = get_file(device_fd);
	if (df == NULL || df->log_fd == LOG_NULL_FILE) {
		const char *name = getenv("TESTNAME");
		if (!name)
			name = "unknown";
		rd_start(device_fd, name, "");
		df = get_file(device_fd);
		assert(df != NULL);
		printf("opened rd, %"LOG_PRI_FILE"\n", df->log_fd);
	}
	// GOOGLE: Don't capture if not in capturing mode.
	if(!IsCapturing()) {
		return;
	}

	pthread_mutex_lock(&write_lock);

	rd_write(device_fd, &val, 4);
	rd_write(device_fd, &val, 4);

	rd_write(device_fd, &type, 4);
	val = ALIGN(sz, 4);
	rd_write(device_fd, &val, 4);
	rd_write(device_fd, buf, sz);

	val = 0;
	rd_write(device_fd, &val, ALIGN(sz, 4) - sz);

	if (wrap_safe()) {
		LOG_SYNC_FILE(df->log_fd);
	}

	pthread_mutex_unlock(&write_lock);
}

unsigned int env2u(const char *name)
{
	const char *str = getenv(name);
	if (!str)
		return 0;
	return strtol(str, NULL, 0);
}

/* in safe mode, sync log file frequently, and insert delays before/after
 * issueibcmds.. useful when we are crashing things and want to be sure to
 * capture as much of the log as possible
 */
unsigned int wrap_safe(void)
{
	static unsigned int val = -1;
	if (val == -1) {
		val = env2u("WRAP_SAFE");
	}
	return val;
}

/* if non-zero, emulate a different gpu-id.  The issueibcmds will be stubbed
 * so we don't actually submit cmds to the gpu.  This is useful to generate
 * cmdstream dumps for different gpu versions for comparision.
 */
unsigned int wrap_gpu_id(void)
{
	static unsigned int val = -1;
	if (val == -1) {
		val = env2u("WRAP_GPU_ID");
	}
	return val;
}

/* defaults to zero if $WRAP_GPU_ID does not end in a ".%d".. */
unsigned int wrap_gpu_id_patchid(void)
{
	static unsigned int val = -1;
	if (val == -1) {
		char *endptr = NULL;
		const char *str = getenv("WRAP_GPU_ID");
		if (str) {
			unsigned int gpuid = strtol(str, &endptr, 0);
			if (endptr[0] == '.') {
				val = strtol(endptr + 1, NULL, 0);
			}
		} else {
			val = 0;
		}
	}
	return val;
}

unsigned int wrap_gmem_size(void)
{
	static unsigned int val = -1;
	if (val == -1) {
		val = env2u("WRAP_GMEM_SIZE");
	}
	return val;
}

/* A hack to reduce the trace size, places a cap on the sizes
 * of individual buffers.
 */
unsigned int wrap_buf_len_cap(void)
{
	static unsigned int val = -1;
	static int first_time = 0;
	if (!first_time) {
		first_time = 1;
		const char *str = getenv("WRAP_BUF_LEN_CAP");
		if (str)
			val = strtol(str, NULL, 0);
	}
	return val;
}

/* Dump ALL buffer objects before each submit, necessary when
 * we want to replay the command stream.
 */
unsigned int wrap_dump_all_bo(void)
{
	static unsigned int val = -1;
	if (val == -1) {
		val = env2u("WRAP_DUMP_ALL_BO");
	}
	return val;
}

void * __rd_dlsym_helper(const char *name)
{
	static void *libc_dl;
#ifdef BIONIC
	static void *libc2d2_dl;
#endif
	void *func;

#ifndef BIONIC
	if (!libc_dl)
		libc_dl = dlopen("/lib/arm-linux-gnueabihf/libc-2.15.so", RTLD_LAZY);
	if (!libc_dl)
		libc_dl = dlopen("/lib/libc-2.16.so", RTLD_LAZY);
#endif
	if (!libc_dl)
		libc_dl = dlopen("libc.so", RTLD_LAZY);

	if (!libc_dl) {
		printf("Failed to dlopen libc: %s\n", dlerror());
		exit(-1);
	}

#if 0
	if (!libc2d2_dl)
		libc2d2_dl = dlopen("libC2D2.so", RTLD_LAZY);

	if (!libc2d2_dl) {
		printf("Failed to dlopen c2d2: %s\n", dlerror());
		exit(-1);
	}
#endif

	func = dlsym(libc_dl, name);

#ifdef BIONIC
	if (!func)
		func = dlsym(libc2d2_dl, name);
#endif

	if (!func) {
		printf("Failed to find %s: %s\n", name, dlerror());
		exit(-1);
	}

	return func;
}

// GOOGLE: Close all opened trace fd
void collect_trace_file(const char* capture_file_path)
{
	char full_path[PATH_MAX];
#if defined (COMPRESS_TRACE)
	snprintf(full_path, PATH_MAX, "%s", capture_file_path);
#else
	snprintf(full_path, PATH_MAX, "%s", capture_file_path);
#endif
	LOGD("full_path is %s", full_path);
	
	char cmd[PATH_MAX];
	snprintf(cmd, PATH_MAX, "touch %s", full_path);
	system(cmd);
	for (int i = 0; i < MAX_DEVICE_FILES; i++)
	{
		struct device_file* df = &device_files[i];
		int fd = 	df->device_fd ;
		if(fd != -1)
		{
			struct device_file *df = get_file(fd);
			if (df == NULL)
					continue;
			LOGD("device_fd %d, log_fd %p closed in collect_trace_file \n", fd, df->log_fd);
			LOG_CLOSE_FILE(df->log_fd);
			df->log_fd = LOG_NULL_FILE;
			snprintf(cmd, PATH_MAX, "cat %s %s > %s", full_path, df->file_name, full_path);
			LOGD("CMD: %s\n", cmd);
			system(cmd);
			// delete the file that has been concatenated. 
			snprintf(cmd, PATH_MAX, "rm %s ", df->file_name);
			LOGD("CMD: %s\n", cmd);
			system(cmd);
        }
	}
}