
#include <osapi/core.h>

enum kxplayer_audio_sample_format {
    kxplayer_audio_sample_format_s16 = 1,
};

enum kxplayer_audio_device_event {
    kxplayer_audio_device_event_hungry = 1,
};

typedef void (*kxplayer_audio_device_notice)(int event);

struct kxplayer_audio_option {
    int sample_rate;
    int sample_format;
    int channels;
    kxplayer_audio_device_notice notice_callback;
};

struct kxplayer_audio_device;

extern int kxplayer_audio_open(struct kxplayer_audio_option* option);
extern void kxplayer_audio_pause(int on);
extern void kxplayer_audio_close(void);
extern os_size kxplayer_audio_write(void* data, os_size length);












