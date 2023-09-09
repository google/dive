#if defined(__ANDROID__)
#    include <android/log.h>

struct Load_log
{
    Load_log() { __android_log_print(ANDROID_LOG_INFO, "libwrap", "Libwrap loaded ..."); }
} load_log;
#endif