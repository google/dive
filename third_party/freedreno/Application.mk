APP_OPTIM := debug
APP_ABI := arm64-v8a armeabi-v7a
# GOOGLE: use dynamic libc++ lib.
APP_STL := c++_shared
APP_CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -DBIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD
APP_CPPFLAGS := -frtti -fexceptions
APP_PLATFORM := android-24
APP_BUILD_SCRIPT := Android.mk
