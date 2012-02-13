
#include <osapi.h>
#include <osapi/thread.h>
#include <osapi/system.h>
#include <libavformat/avformat.h>

#include "device.h"
#include "api/kxplayer/player.h"
#include "api/kxplayer/agent.h"
#include "api/kxplayer/device.h"

int kxplayer_createplayer(void);
void kxplayer_releaseplayer(void);

static struct os_thread_mutex* g_global_lock = NULL;

int kxplayer_initialize(void) {
    if (osapi_initialize() != 0) {
        fprintf(stderr, "failed to initialize osapi.\n");
        return -1;
    }
    
    if (internal_device_initialize() != 0) {
        fprintf(stderr, "failed to initialize device.\n");
        goto err1;
    }
    
    if ((g_global_lock = os_thread_mutex_create()) == NULL) {
        fprintf(stderr, "unable to create lock for player.\n");
        goto err2;
    }
    
    if (kxplayer_createplayer() != 0) {
        fprintf(stderr, "unable to create player.\n");
        goto err3;
    }
    
    av_register_all();
    avformat_network_init();
    
    return 0;
    
err3:
    os_thread_mutex_release(g_global_lock);
err2:
    internal_device_terminate();
err1:
    osapi_terminate();
    return -1;
}

void kxplayer_terminate(void) {
    kxplayer_releaseplayer();
    if (g_global_lock != NULL) {
        os_thread_mutex_release(g_global_lock);
        g_global_lock = NULL;
    }
    avformat_network_deinit();
    internal_device_terminate();
    osapi_terminate();
}


/* =============================================================================
 * Player Implementaion
 */

struct kxplayer_player {
    struct kxplayer_agent* agent;
} g_player;


static void player_receive_callback(void* data, os_size length, void* userdata) {
    os_size size = 0;
    while (size < length) {
        size += kxplayer_audio_write(data + size, length-size);
        if (size < length) {
#if 0
            fprintf(stdout, "wait for buffer(%lu).\n", length-size);
#endif
            os_sleep(1000);
        }
    }
}

static void player_finish_callback(void* userdata) {
    
}

static void player_notice_callback(int event) {
    int state = kxplayer_agent_getstate(g_player.agent);
    switch (state) {
        case kxplayer_agent_state_idle:
            fprintf(stdout, "close audio.\n");
            kxplayer_audio_pause(1);
            break;
        case kxplayer_agent_state_started:
            break;
        default:
            break;
    }
}


int kxplayer_createplayer(void) {
    os_thread_mutex_lock(g_global_lock);
    
    struct kxplayer_player* player = &g_player;
    
    struct kxplayer_agent_option agent_option;
    agent_option.userdata = player;
    agent_option.receive_callback = player_receive_callback;
    agent_option.finish_callback = player_finish_callback;
    player->agent = kxplayer_agent_create(&agent_option);
    if (player->agent == NULL) {
        fprintf(stderr, "failed to create player agent.\n");
        goto err1;
    } 
        
    os_thread_mutex_unlock(g_global_lock);
    return 0;
    
err1:
    os_thread_mutex_unlock(g_global_lock);
    return -1;
}

void kxplayer_releaseplayer(void) {
    kxplayer_agent_release(g_player.agent);
    g_player.agent = NULL;
    kxplayer_audio_close();
}


int kxplayer_play(const char* uri) {
    OS_ASSERT(uri!=NULL, -1);
        
    struct kxplayer_avcontext context;
    if (kxplayer_agent_open(g_player.agent, uri, &context) != 0) {
        fprintf(stderr, "unable to open URI %s\n", uri);
        return -1;
    }
    
    struct kxplayer_audio_option option;
    option.sample_rate = context.audio_sample_rate;
    option.sample_format = context.audio_sample_format;
    option.channels = context.audio_channels;
    option.notice_callback = player_notice_callback;
    if (kxplayer_audio_open(&option) != 0) {
        fprintf(stderr, "failed to open audio device.\n");
        return -1;
    }
    
    if (kxplayer_agent_start(g_player.agent) != 0) {
        fprintf(stderr, "unable to start agent.\n");
        return -1;
    }
    
    return 0;
}


















