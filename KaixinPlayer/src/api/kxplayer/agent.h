
#ifndef kxplayer_agent_h
#define kxplayer_agent_h

#include <osapi/core.h>

enum kxplayer_agent_state {
    kxplayer_agent_state_idle           = 0,
    kxplayer_agent_state_open           = 1,
    kxplayer_agent_state_started        = 2,
    kxplayer_agent_state_receving_data  = 3,
    kxplayer_agent_state_paused         = 4,
    kxplayer_agent_state_aborted        = 5,
};

struct kxplayer_avcontext {
    int has_audio;
    int audio_sample_rate;
    int audio_sample_format;
    int audio_channels;
    
    int has_video;
};

struct kxplayer_agent_option {
    void (*receive_callback)(void* data, os_size size, void* userdata);
    void (*finish_callback)(void* userdata);
    void* userdata;
};
struct kxplayer_agent;

extern struct kxplayer_agent* kxplayer_agent_create(const struct kxplayer_agent_option* option);
extern void kxplayer_agent_release(struct kxplayer_agent* agent);
extern int kxplayer_agent_open(struct kxplayer_agent* agent, 
                               const char* uri, 
                               struct kxplayer_avcontext* context);
extern int kxplayer_agent_start(struct kxplayer_agent* agent);
extern int kxplayer_agent_pause(struct kxplayer_agent* agent);
extern int kxplayer_agent_resume(struct kxplayer_agent* agent);
extern int kxplayer_agent_abort(struct kxplayer_agent* agent);
extern int kxplayer_agent_getstate(struct kxplayer_agent* agent);

#endif