

#include "api/kxplayer/config.h"
#include "api/kxplayer/device.h"
#include "device.h"

#if KXPLAYER_USE_SDL

#include <osapi/buffer.h>
#include <osapi/thread.h>
#include <libavformat/avformat.h>

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#define SDL_AUDIO_BUFFER_SIZE 1024

#define SRC_KXPLAYER_FRAME_MAX       64
#define SRC_KXPLAYER_FRAME_MAXSIZE   (1024*4)

static struct kxplayer_audio_device {
    struct os_loopbuf* buffer;
    kxplayer_audio_device_notice notice_callback;
} g_audio_device;
static int g_audio_open = 0;
static struct os_thread_mutex* g_global_lock = NULL;


int internal_device_initialize(void) {
    g_global_lock = os_thread_mutex_create();
    if (g_global_lock == NULL) {
        fprintf(stderr, "failed to create global lock for audio device.\n");
        return -1;
    }
    
    if(SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        os_thread_mutex_release(g_global_lock);
        g_global_lock = NULL;
        return -1;
    }

    return 0;
}

void internal_device_terminate(void) {
    if (g_global_lock != NULL) {
        os_thread_mutex_release(g_global_lock);
        g_global_lock = NULL;
    }
    
    SDL_Quit();
}

static int translate_sdl_sample_format(int format) {
    switch (format) {
        case kxplayer_audio_sample_format_s16:
            return AUDIO_S16;
        default:
            return AUDIO_S16;
    }
}

static void sdl_audio_callback(void *userdata, Uint8 *stream, int length) {
    OS_ASSERT(length<=SRC_KXPLAYER_FRAME_MAXSIZE, ;);
    
    char buf[SRC_KXPLAYER_FRAME_MAXSIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    if (!os_loopbuf_isempty(g_audio_device.buffer)) {
        os_size size = os_loopbuf_read(g_audio_device.buffer, buf, length);
#if 0
        fprintf(stdout, "play size %lu\n", size);
#endif
        memcpy(stream, buf, size);
        if (size < length) {
            memset(stream+size, 0, length-size);
#if 0
            fprintf(stderr, "Not enough audio data(%lu).\n", length-size);
#endif
            if (g_audio_device.notice_callback != NULL) {
                g_audio_device.notice_callback(kxplayer_audio_device_event_hungry);
            }
        }
    } else {
        memset(stream, 0, length);
#if 0
        fprintf(stderr, "Not enough audio data(%d).\n", length);
#endif
        if (g_audio_device.notice_callback != NULL) {
            g_audio_device.notice_callback(kxplayer_audio_device_event_hungry);
        }
    }
}


int kxplayer_audio_open(struct kxplayer_audio_option* option) {
    os_thread_mutex_lock(g_global_lock);
    if (g_audio_open) {
        fprintf(stderr, "Audio device is already open.\n");
        os_thread_mutex_unlock(g_global_lock);
        return -1;
    }
    
    OS_ASSERT(option!=NULL, -1);
    
    g_audio_device.buffer = os_loopbuf_create(SRC_KXPLAYER_FRAME_MAX*SRC_KXPLAYER_FRAME_MAXSIZE);
    if (g_audio_device.buffer == NULL) {
        fprintf(stderr, "failed to create buffer.\n");
        os_thread_mutex_unlock(g_global_lock);
        return -1;
    }
    g_audio_device.notice_callback = option->notice_callback;
    
    SDL_AudioSpec   wanted_spec, spec;
    wanted_spec.freq = option->sample_rate;
    wanted_spec.format = translate_sdl_sample_format(option->sample_format);
    wanted_spec.channels = option->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = NULL;
    if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        goto err1;
    }
    SDL_PauseAudio(0);
    
    g_audio_open = 1;
    os_thread_mutex_unlock(g_global_lock);
    return 0;
    
err1:
    os_loopbuf_release(g_audio_device.buffer);
    g_audio_device.buffer = NULL;
    os_thread_mutex_unlock(g_global_lock);
    return 0;
}


void kxplayer_audio_close(void) {
    os_thread_mutex_lock(g_global_lock);
    if (!g_audio_open) {
        os_thread_mutex_unlock(g_global_lock);
        return;
    }
    if (g_audio_device.buffer != NULL) {
        os_loopbuf_release(g_audio_device.buffer);
    }
    SDL_CloseAudio();
    g_audio_open = 0;
    os_thread_mutex_unlock(g_global_lock);
}

os_size kxplayer_audio_write(void* data, os_size length) {
    OS_ASSERT(data!=NULL, 0);
    OS_ASSERT(length>0, 0);
    os_thread_mutex_lock(g_global_lock);
    if (!g_audio_open) {
        os_thread_mutex_unlock(g_global_lock);
        return 0;
    }
    os_thread_mutex_unlock(g_global_lock);
    
    return os_loopbuf_write(g_audio_device.buffer, data, length);
}

void kxplayer_audio_pause(int on) {
    os_thread_mutex_lock(g_global_lock);
    if (!g_audio_open) {
        os_thread_mutex_unlock(g_global_lock);
        return;
    }
    SDL_PauseAudio(on);
}

#endif


