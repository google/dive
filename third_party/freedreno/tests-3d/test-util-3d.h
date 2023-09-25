/*
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
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

#ifndef TEST_UTIL_3D_H_
#define TEST_UTIL_3D_H_


#include "test-util-common.h"

#include <EGL/egl.h>
#ifndef GL_ES_VERSION_2_0
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#endif

#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/*****************************************************************************/

#define ECHK(x) do { \
		EGLBoolean status; \
		DEBUG_MSG(">>> %s", #x); \
		RD_WRITE_SECTION(RD_CMD, #x, strlen(#x)); \
		status = (EGLBoolean)(x); \
		if (!status) { \
			EGLint err = eglGetError(); \
			ERROR_MSG("<<< %s: failed: 0x%04x (%s)", #x, err, eglStrError(err)); \
			exit(-1); \
		} \
		DEBUG_MSG("<<< %s: succeeded", #x); \
	} while (0)

#define GCHK(x) do { \
		GLenum err; \
		DEBUG_MSG(">>> %s", #x); \
		RD_WRITE_SECTION(RD_CMD, #x, strlen(#x)); \
		fflush(stdout); \
		x; \
		err = glGetError(); \
		if (err != GL_NO_ERROR) { \
			ERROR_MSG("<<< %s: failed: 0x%04x (%s)", #x, err, glStrError(err)); \
			exit(-1); \
		} \
		DEBUG_MSG("<<< %s: succeeded", #x); \
		fflush(stdout); \
		usleep(1000); \
	} while (0)

#define ENUM(x) case x: return #x
static inline const char *
typename(GLenum type)
{
	switch(type) {
	ENUM(GL_BYTE);
	ENUM(GL_UNSIGNED_BYTE);
	ENUM(GL_SHORT);
	ENUM(GL_UNSIGNED_SHORT);
	ENUM(GL_INT);
	ENUM(GL_UNSIGNED_INT);
	ENUM(GL_FIXED);
	ENUM(GL_FLOAT);
#ifdef GL_UNSIGNED_INT_10_10_10_2_OES
	ENUM(GL_UNSIGNED_INT_10_10_10_2_OES);
#endif
#ifdef GL_INT_10_10_10_2_OES
	ENUM(GL_INT_10_10_10_2_OES);
#endif
	ENUM(GL_UNSIGNED_SHORT_5_6_5);
	ENUM(GL_UNSIGNED_SHORT_4_4_4_4);
	ENUM(GL_UNSIGNED_SHORT_5_5_5_1);
#ifdef GL_HALF_FLOAT_OES
	ENUM(GL_HALF_FLOAT_OES);
#endif
#ifdef GL_UNSIGNED_INT_2_10_10_10_REV_EXT
	ENUM(GL_UNSIGNED_INT_2_10_10_10_REV_EXT);
#endif
#ifdef GL_UNSIGNED_INT_2_10_10_10_REV
	ENUM(GL_UNSIGNED_INT_2_10_10_10_REV);
#endif
#ifdef GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT
	ENUM(GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT);
#endif
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT
	ENUM(GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT);
#endif
#ifdef GL_HALF_FLOAT
	ENUM(GL_HALF_FLOAT);
#endif
#ifdef GL_UNSIGNED_INT_10F_11F_11F_REV
	ENUM(GL_UNSIGNED_INT_10F_11F_11F_REV);
#endif
#ifdef GL_UNSIGNED_INT_5_9_9_9_REV
	ENUM(GL_UNSIGNED_INT_5_9_9_9_REV);
#endif
#ifdef GL_UNSIGNED_INT_24_8
	ENUM(GL_UNSIGNED_INT_24_8);
#endif
#ifdef GL_FLOAT_32_UNSIGNED_INT_24_8_REV
	ENUM(GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
#endif
	}
	ERROR_MSG("invalid type: %04x", type);
	exit(1);
	return NULL;
}

static inline const char *
formatname(GLenum format)
{
	switch (format) {
	ENUM(GL_RGB);
	ENUM(GL_RGBA);
	ENUM(GL_ALPHA);
	ENUM(GL_LUMINANCE);
	ENUM(GL_LUMINANCE_ALPHA);
	ENUM(GL_DEPTH_COMPONENT);
#ifdef GL_R8
	ENUM(GL_R8);
#endif
#ifdef GL_BGRA_EXT
	ENUM(GL_BGRA_EXT);
#endif
#ifdef GL_RGB8
	ENUM(GL_RGB8);
#endif
#ifdef GL_RG
	ENUM(GL_RG);
#endif
#ifdef GL_RG_INTEGER
	ENUM(GL_RG_INTEGER);
#endif
#ifdef GL_RED
	ENUM(GL_RED);
#endif
#ifdef GL_RED_INTEGER
	ENUM(GL_RED_INTEGER);
#endif
#ifdef GL_RGB_INTEGER
	ENUM(GL_RGB_INTEGER);
#endif
#ifdef GL_RGBA_INTEGER
	ENUM(GL_RGBA_INTEGER);
#endif
#ifdef GL_RGB565
	ENUM(GL_RGB565);
#endif
#ifdef GL_RGBA4
	ENUM(GL_RGBA4);
#endif
#ifdef GL_RGB10_A2
	ENUM(GL_RGB10_A2);
#endif
#ifdef GL_UNSIGNED_INT_10F_11F_11F_REV
	ENUM(GL_UNSIGNED_INT_10F_11F_11F_REV);
#endif
#ifdef GL_UNSIGNED_INT_5_9_9_9_REV
	ENUM(GL_UNSIGNED_INT_5_9_9_9_REV);
#endif
#ifdef GL_RGB8_SNORM
	ENUM(GL_RGB8_SNORM);
#endif
#ifdef GL_RGBA8_SNORM
	ENUM(GL_RGBA8_SNORM);
#endif
#ifdef GL_R11F_G11F_B10F
	ENUM(GL_R11F_G11F_B10F);
#endif
#ifdef GL_RGB9_E5
	ENUM(GL_RGB9_E5);
#endif
#ifdef GL_RGB8I
	ENUM(GL_RGB8I);
#endif
#ifdef GL_RGB8UI
	ENUM(GL_RGB8UI);
#endif
#ifdef GL_RGB16I
	ENUM(GL_RGB16I);
#endif
#ifdef GL_RGB16UI
	ENUM(GL_RGB16UI);
#endif
#ifdef GL_RGB16F
	ENUM(GL_RGB16F);
#endif
#ifdef GL_RGBA16F
	ENUM(GL_RGBA16F);
#endif
#ifdef GL_RGB32I
	ENUM(GL_RGB32I);
#endif
#ifdef GL_RGB32UI
	ENUM(GL_RGB32UI);
#endif
#ifdef GL_RGBA32UI
	ENUM(GL_RGBA32UI);
#endif
#ifdef GL_RGB32F
	ENUM(GL_RGB32F);
#endif
#ifdef GL_RGBA32F
	ENUM(GL_RGBA32F);
#endif
#ifdef GL_R16F
	ENUM(GL_R16F);
#endif
#ifdef GL_R16I
	ENUM(GL_R16I);
#endif
#ifdef GL_R16UI
	ENUM(GL_R16UI);
#endif
#ifdef GL_RG16F
	ENUM(GL_RG16F);
#endif
#ifdef GL_RG16I
	ENUM(GL_RG16I);
#endif
#ifdef GL_RG16UI
	ENUM(GL_RG16UI);
#endif
#ifdef GL_R32F
	ENUM(GL_R32F);
#endif
#ifdef GL_R32I
	ENUM(GL_R32I);
#endif
#ifdef GL_R32UI
	ENUM(GL_R32UI);
#endif
#ifdef GL_RG32F
	ENUM(GL_RG32F);
#endif
#ifdef GL_RG32I
	ENUM(GL_RG32I);
#endif
#ifdef GL_RG32UI
	ENUM(GL_RG32UI);
#endif
#ifdef GL_R8I
	ENUM(GL_R8I);
#endif
#ifdef GL_R8UI
	ENUM(GL_R8UI);
#endif
#ifdef GL_RG8
	ENUM(GL_RG8);
#endif
#ifdef GL_RG8I
	ENUM(GL_RG8I);
#endif
#ifdef GL_RG8UI
	ENUM(GL_RG8UI);
#endif
#ifdef GL_RG8_SNORM
	ENUM(GL_RG8_SNORM);
#endif
#ifdef GL_SRGB_EXT
	ENUM(GL_SRGB_EXT);
#endif
#ifdef GL_SRGB_ALPHA_EXT
	ENUM(GL_SRGB_ALPHA_EXT);
#endif
#ifdef GL_SRGB8_ALPHA8_EXT
	ENUM(GL_SRGB8_ALPHA8_EXT);
#endif
#ifdef GL_SRGB
	ENUM(GL_SRGB);
#endif
#ifdef GL_SRGB8
	ENUM(GL_SRGB8);
#endif
#ifdef GL_SRGB8_ALPHA8
	ENUM(GL_SRGB8_ALPHA8);
#endif
#ifdef GL_RGB10_A2UI
	ENUM(GL_RGB10_A2UI);
#endif
#ifdef GL_RGBA8UI
	ENUM(GL_RGBA8UI);
#endif
#ifdef GL_RGB5_A1
	ENUM(GL_RGB5_A1);
#endif
#ifdef GL_RGBA8
	ENUM(GL_RGBA8);
#endif
#ifdef GL_RGBA8I
	ENUM(GL_RGBA8I);
#endif
#ifdef GL_RGBA16UI
	ENUM(GL_RGBA16UI);
#endif
#ifdef GL_RGBA16I
	ENUM(GL_RGBA16I);
#endif
#ifdef GL_RGBA32I
	ENUM(GL_RGBA32I);
#endif
#ifdef GL_R8_SNORM
	ENUM(GL_R8_SNORM);
#endif
#ifdef GL_DEPTH_COMPONENT16
	ENUM(GL_DEPTH_COMPONENT16);
#endif
#ifdef GL_DEPTH_COMPONENT24
	ENUM(GL_DEPTH_COMPONENT24);
#endif
#ifdef GL_DEPTH_COMPONENT32F
	ENUM(GL_DEPTH_COMPONENT32F);
#endif
#ifdef GL_DEPTH24_STENCIL8
	ENUM(GL_DEPTH24_STENCIL8);
#endif
#ifdef GL_DEPTH32F_STENCIL8
	ENUM(GL_DEPTH32F_STENCIL8);
#endif
#ifdef GL_DEPTH_STENCIL
	ENUM(GL_DEPTH_STENCIL);
#endif
	ENUM(GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
	ENUM(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
	}
	ERROR_MSG("invalid format: %04x", format);
	exit(1);
	return NULL;
}

static inline const char *
textypename(GLenum type)
{
	switch (type) {
	ENUM(GL_TEXTURE_2D);
#ifdef GL_TEXTURE_2D_ARRAY
	ENUM(GL_TEXTURE_2D_ARRAY);
#endif
#ifdef GL_TEXTURE_3D
	ENUM(GL_TEXTURE_3D);
#endif
	ENUM(GL_TEXTURE_CUBE_MAP);
	}
	ERROR_MSG("invalid type: %04x", type);
	exit(1);
	return NULL;
}

static inline const char *
compname(GLenum compf)
{
	switch (compf) {
	ENUM(GL_NEVER);
	ENUM(GL_LESS);
	ENUM(GL_EQUAL);
	ENUM(GL_LEQUAL);
	ENUM(GL_GREATER);
	ENUM(GL_NOTEQUAL);
	ENUM(GL_GEQUAL);
	ENUM(GL_ALWAYS);
	}
	ERROR_MSG("invalid func: %04x", compf);
	exit(1);
	return NULL;
}

static char *
eglStrError(EGLint error)
{
	switch (error) {
	ENUM(EGL_SUCCESS);
	ENUM(EGL_NOT_INITIALIZED);
	ENUM(EGL_BAD_ALLOC);
	ENUM(EGL_BAD_CONFIG);
	ENUM(EGL_BAD_PARAMETER);
	ENUM(EGL_BAD_MATCH);
	ENUM(EGL_BAD_ATTRIBUTE);
	default:
		return "UNKNOWN";
	}
}

static char *
glStrError(GLenum error)
{
	switch (error) {
	// TODO
	ENUM(GL_INVALID_ENUM);
	ENUM(GL_INVALID_OPERATION);
	ENUM(GL_INVALID_FRAMEBUFFER_OPERATION);
	default:
		return "UNKNOWN";
	}
}

#undef ENUM

#if defined(SUPPORT_X11)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>
#elif defined(SUPPORT_WAYLAND)
#  include <wayland-server.h>
#  include <wayland-client-protocol.h>
#  include <wayland-egl.h>
#endif

#if defined(SUPPORT_WAYLAND)
static struct wl_display *wl_display;
#else
static EGLNativeDisplayType native_dpy;
#endif

#ifndef EGL_KHR_platform_android
#define EGL_KHR_platform_android 1
#define EGL_PLATFORM_ANDROID_KHR          0x3141
#endif /* EGL_KHR_platform_android */

static EGLDisplay
get_display(void)
{
	EGLDisplay display;
	EGLint egl_major, egl_minor;

#if defined(BIONIC)
	native_dpy = EGL_DEFAULT_DISPLAY;
	display = eglGetDisplay(native_dpy);
#elif defined(SUPPORT_WAYLAND)
	wl_display = wl_display_connect(NULL);
	display = eglGetDisplay((EGLNativeDisplayType) wl_display);
#else
	native_dpy = XOpenDisplay(NULL);
	display = eglGetDisplay(native_dpy);
#endif
	if (display == EGL_NO_DISPLAY) {
		ERROR_MSG("No display found!");
		exit(-1);
	}

	ECHK(eglInitialize(display, &egl_major, &egl_minor));

	DEBUG_MSG("Using display %p with EGL version %d.%d",
			display, egl_major, egl_minor);

	DEBUG_MSG("EGL Version \"%s\"", eglQueryString(display, EGL_VERSION));
	DEBUG_MSG("EGL Vendor \"%s\"", eglQueryString(display, EGL_VENDOR));
	DEBUG_MSG("EGL Extensions \"%s\"", eglQueryString(display, EGL_EXTENSIONS));

	return display;
}

#if defined(SUPPORT_WAYLAND)
static struct wl_compositor *compositor;
static struct wl_shell *shell;

static void global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
									const char *interface, uint32_t version)
{
	if (strcmp(interface, "wl_compositor") == 0)
		compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	else if (strcmp(interface, "wl_shell") == 0)
		shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
}

static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{

}

static const struct wl_registry_listener registry_listener = {
	global_registry_handler,
	global_registry_remover
};

static void get_server_references()
{
	struct wl_registry *wl_registry = wl_display_get_registry(wl_display);
	wl_registry_add_listener(wl_registry, &registry_listener, NULL);

	wl_display_dispatch(wl_display);
	wl_display_roundtrip(wl_display);

	if (compositor == NULL || shell == NULL) {
		ERROR_MSG("Can't find compositor or shell");
		exit(-1);
	}
}
#endif

static EGLSurface make_window(EGLDisplay display, EGLConfig config, int width, int height)
{
	EGLSurface surface;
#if defined(BIONIC)
	EGLint pbuffer_attribute_list[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LARGEST_PBUFFER, EGL_TRUE,
		EGL_NONE
	};
	ECHK(surface = eglCreatePbufferSurface(display, config, pbuffer_attribute_list));
#elif defined(SUPPORT_WAYLAND)
	struct wl_surface *compositor_surface;
	struct wl_shell_surface *shell_surface;
	struct wl_egl_window *egl_window;

	get_server_references();

	compositor_surface = wl_compositor_create_surface(compositor);
	if (compositor_surface == NULL) {
		ERROR_MSG("Can't create surface");
		exit(-1);
	}

	shell_surface = wl_shell_get_shell_surface(shell, compositor_surface);
	wl_shell_surface_set_toplevel(shell_surface);

	egl_window = wl_egl_window_create(compositor_surface, width, height);
	if (egl_window == EGL_NO_SURFACE) {
		ERROR_MSG("Can't create egl window");
		exit(-1);
	}

	surface = eglCreateWindowSurface(display, config, egl_window, NULL);
#else
	XVisualInfo *visInfo, visTemplate;
	int num_visuals;
	Window root, xwin;
	XSetWindowAttributes attr;
	unsigned long mask;
	EGLint vid;
	const char *title = "egl";

	ECHK(eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &vid));

	/* The X window visual must match the EGL config */
	visTemplate.visualid = vid;
	visInfo = XGetVisualInfo(native_dpy, VisualIDMask, &visTemplate, &num_visuals);
	if (!visInfo) {
		ERROR_MSG("failed to get an visual of id 0x%x", vid);
		exit(-1);
	}

	root = RootWindow(native_dpy, DefaultScreen(native_dpy));

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(native_dpy,
			root, visInfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	xwin = XCreateWindow(native_dpy, root, 0, 0, width, height,
			0, visInfo->depth, InputOutput, visInfo->visual, mask, &attr);
	if (!xwin) {
		ERROR_MSG("failed to create a window");
		exit (-1);
	}

	XFree(visInfo);

	/* set hints and properties */
	{
		XSizeHints sizehints;
		sizehints.x = 0;
		sizehints.y = 0;
		sizehints.width  = width;
		sizehints.height = height;
		sizehints.flags = USSize | USPosition;
		XSetNormalHints(native_dpy, xwin, &sizehints);
		XSetStandardProperties(native_dpy, xwin,
				title, title, None, (char **) NULL, 0, &sizehints);
	}

	XMapWindow(native_dpy, xwin);

	surface = eglCreateWindowSurface(display, config, xwin, NULL);
#endif
	return surface;
}

static void dump_bmp(EGLDisplay display, EGLSurface surface, const char *filename)
{
	GLint width, height;
	void *buf;

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	buf = malloc(width * height * 4);

	GCHK(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf));

	wrap_bmp_dump(buf, width, height, width*4, filename);
}

static void readback(void)
{
	char buf[64];
	GCHK(glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf));
}

#ifdef GL_READ_BUFFER
static void readbuf(GLenum readbuf)
{
	char buf[64];
	GLenum fmt, type;
	int saved;

	switch (readbuf) {
#ifdef GL_STENCIL_INDEX
	case GL_STENCIL_INDEX:
#endif
	case GL_DEPTH_COMPONENT:
		fmt = readbuf;
		readbuf = GL_BACK;
		type = GL_FLOAT;
		break;
	case GL_DEPTH_STENCIL:
		fmt = readbuf;
		readbuf = GL_BACK;
		type = GL_UNSIGNED_INT_24_8;
		break;
	default:
		fmt = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		break;
	}

	GCHK(glGetIntegerv(GL_READ_BUFFER, &saved));
	DEBUG_MSG("readbuf=%04x, fmt=%s, type=%s", readbuf, formatname(fmt), typename(type));
	GCHK(glReadBuffer(readbuf));
	GCHK(glReadPixels(0, 0, 1, 1, fmt, type, buf));
	GCHK(glReadBuffer(saved));
}
#endif

static GLuint
get_shader(GLenum stage, const char *stage_name, const char *source)
{
	GLuint shader;
	GLint ret;

	DEBUG_MSG("%s shader:\n%s", stage_name, source);

	GCHK(shader = glCreateShader(stage));

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

	return shader;
}

static GLuint
get_program(const char *vertex_shader_source, const char *fragment_shader_source)
{
	GLuint vertex_shader, fragment_shader, program;

	DEBUG_MSG("vertex shader:\n%s", vertex_shader_source);
	DEBUG_MSG("fragment shader:\n%s", fragment_shader_source);

	RD_WRITE_SECTION(RD_VERT_SHADER,
			vertex_shader_source, strlen(vertex_shader_source));
	RD_WRITE_SECTION(RD_FRAG_SHADER,
			fragment_shader_source, strlen(fragment_shader_source));

	vertex_shader = get_shader(GL_VERTEX_SHADER, "vertex", vertex_shader_source);
	fragment_shader = get_shader(GL_FRAGMENT_SHADER, "fragment", fragment_shader_source);

	GCHK(program = glCreateProgram());

	GCHK(glAttachShader(program, vertex_shader));
	GCHK(glAttachShader(program, fragment_shader));

	return program;
}

#ifdef GL_COMPUTE_SHADER
static GLuint
get_compute_program(const char *shader_source)
{
	GLuint shader, program;

	DEBUG_MSG("compute shader:\n%s", shader_source);

	RD_WRITE_SECTION(RD_FRAG_SHADER,
			shader_source, strlen(shader_source));

	shader = get_shader(GL_COMPUTE_SHADER, "compute", shader_source);

	GCHK(program = glCreateProgram());
	GCHK(glAttachShader(program, shader));

	return program;
}
#endif

/* ***** GL_OES_get_program_binary extension: ****************************** */
/* Accepted by the <pname> parameter of GetProgramiv:
 */
#define GL_PROGRAM_BINARY_LENGTH_OES	0x8741
/* Accepted by the <pname> parameter of GetBooleanv, GetIntegerv, and GetFloatv:
 */
#define GL_NUM_PROGRAM_BINARY_FORMATS_OES	0x87FE
#define GL_PROGRAM_BINARY_FORMATS_OES	0x87FF

void glGetProgramBinaryOES(GLuint program, GLsizei bufSize, GLsizei *length,
		GLenum *binaryFormat, GLvoid *binary);
void glProgramBinaryOES(GLuint program, GLenum binaryFormat,
		const GLvoid *binary, GLint length);
/* ************************************************************************* */

static inline void
hexdump(const void *data, int size)
{
	unsigned char *buf = (void *) data;
	char alpha[17];
	int i;

	for (i = 0; i < size; i++) {
		if (!(i % 16))
			printf("\t\t\t%08X", (unsigned int) i);
		if (!(i % 4))
			printf(" ");

		if (((void *) (buf + i)) < ((void *) data)) {
			printf("   ");
			alpha[i % 16] = '.';
		} else {
			printf(" %02x", buf[i]);

			if (isprint(buf[i]) && (buf[i] < 0xA0))
				alpha[i % 16] = buf[i];
			else
				alpha[i % 16] = '.';
		}

		if ((i % 16) == 15) {
			alpha[16] = 0;
			printf("\t|%s|\n", alpha);
		}
	}

	if (i % 16) {
		for (i %= 16; i < 16; i++) {
			printf("   ");
			alpha[i] = '.';

			if (i == 15) {
				alpha[16] = 0;
				printf("\t|%s|\n", alpha);
			}
		}
	}
}

static void
link_program(GLuint program)
{
	GLint ret, len;
	GLenum binary_format;
	void *binary;

	GCHK(glLinkProgram(program));

	GCHK(glGetProgramiv(program, GL_LINK_STATUS, &ret));
	if (!ret) {
		char *log;

		ERROR_MSG("program linking failed!:");
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(program, ret, NULL, log);
			printf("%s", log);
		}
		exit(-1);
	}

	DEBUG_MSG("program linking succeeded!");

	GCHK(glUseProgram(program));

#ifdef BIONIC
	/* dump program binary: */
	// TODO move this into wrap-gles.c .. just putting it here for now
	// since I haven't created wrap-gles.c yet
	GCHK(glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH_OES, &len));
	binary = calloc(1, len);
	GCHK(glGetProgramBinaryOES(program, len, &ret, &binary_format, binary));
	DEBUG_MSG("program dump: len=%d, actual len=%d", len, ret);
	hexdump(binary, len);
	RD_WRITE_SECTION(RD_PROGRAM, binary, len);
	free(binary);
#endif
}

static inline unsigned int env2u(const char *name)
{
	const char *str = getenv(name);
	if (!str)
		return 0;
	return strtol(str, NULL, 0);
}

#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

static inline unsigned
u_minify(unsigned value, unsigned levels)
{
    return MAX2(1, value >> levels);
}

#endif /* TEST_UTIL_3D_H_ */
