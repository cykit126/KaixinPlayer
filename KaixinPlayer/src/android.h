
#ifndef SRC_KaixinPlayer_android_h
#define SRC_KaixinPlayer_android_h

#include <osapi/core.h>

#if defined(OSAPI_ANDROID)

#include <android/log.h>

#define KXPLAYER_ANDROID_LOGTAG "kxplayer_native"

#define KXPLAYER_LOG(level, ...) \
    __android_log_print((level), KXPLAYER_ANDROID_LOGTAG, __VA_ARGS__)

#endif

#endif
