
#include "api/kxplayer/config.h"
#include "api/kxplayer/device.h"
#include "device.h"

#include <osapi/core.h>
#include <osapi/thread.h>

#if defined (OSAPI_ANDROID)

#include <android/log.h>
#include "android.h"

struct os_thread_mutex* g_lock = NULL;
struct kxplayer_audio_option g_option;
static int g_init = 0;

int internal_device_initialize(void) {
    g_lock = os_thread_mutex_create();
    if (g_lock == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, KXPLAYER_ANDROID_LOGTAG, "failed to create lock.\n");
        return -1;
    }
    g_init = 0;
    return 0;
}

void internal_device_terminate(void) {
    if (g_lock != NULL) {
        os_thread_mutex_release(g_lock);
        g_lock = NULL;
    }
}

int kxplayer_audio_open(struct kxplayer_audio_option* option) {
    os_thread_mutex_lock(g_lock);
    if (!g_init) {
        os_thread_mutex_release(g_lock);
        return -1;
    }
    return 0;
}


void kxplayer_audio_pause(int on) {
    __android_log_print(ANDROID_LOG_ERROR, KXPLAYER_ANDROID_LOGTAG, "Not implemented.\n");
}


void kxplayer_audio_close(void) {
    __android_log_print(ANDROID_LOG_ERROR, KXPLAYER_ANDROID_LOGTAG, "Not implemented.\n");
}


os_size kxplayer_audio_write(void* data, os_size length) {
    __android_log_print(ANDROID_LOG_ERROR, KXPLAYER_ANDROID_LOGTAG, "Not implemented.\n");
    return 0;
}

#endif

