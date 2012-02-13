
#include <osapi/buffer.h>
#include <osapi/thread.h>
#include <osapi/memory.h>
#include <osapi/log.h>

#include <libavformat/avformat.h>

#include "api/kxplayer/agent.h"
#include "api/kxplayer/device.h"

struct kxplayer_agent {
    struct kxplayer_agent_option option;
    struct os_thread_event* read_wait;
    struct os_thread_mutex* lock;
    os_int32 state;
    
    /*  */
    AVFormatContext* format_ctx;
    int audio_stream;
};

static int translate_sample_format(int format) {
    switch (format) {
        case AV_SAMPLE_FMT_S16:
        default:
            return kxplayer_audio_sample_format_s16;
    }
}

struct kxplayer_agent* kxplayer_agent_create(const struct kxplayer_agent_option* option) {
    OS_ASSERT(option!=NULL, NULL);
    
    struct kxplayer_agent* agent = (struct kxplayer_agent*) os_calloc(sizeof(struct kxplayer_agent), 1);
    OS_ASSERT(agent!=NULL, NULL);
    
    memcpy(&agent->option, option, sizeof(*option));
    agent->state = kxplayer_agent_state_idle;
        
    if ((agent->read_wait = os_thread_event_create()) == NULL) {
        OS_LOG(os_log_error, "failed to create thread event.\n");
        goto err1;
    }
    
    if ((agent->lock = os_thread_mutex_create()) == NULL) {
        OS_LOG(os_log_error, "failed to create thread lock.\n");
        goto err2;
    }
    
    return agent;
    
err2:
    os_thread_event_release(agent->read_wait);
err1:
    os_free(agent);
    return NULL;
}


void kxplayer_agent_release(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, ;);
    
    if (agent->read_wait != NULL) {
        os_thread_event_release(agent->read_wait);
        agent->read_wait = NULL;
    }
    
    if (agent->lock != NULL) {
        os_thread_mutex_release(agent->lock);
        agent->lock = NULL;
    }
}

int kxplayer_agent_open(struct kxplayer_agent* agent, 
                           const char* uri, 
                           struct kxplayer_avcontext* context) {
    //OS_ASSERT(agent!=NULL, -1);
    //OS_ASSERT(uri!=NULL, -1);
    if (agent == NULL) {
        OS_LOG(os_log_error, "agent is NULL.");
        return -1;
    }
    if (uri == NULL) {
        OS_LOG(os_log_error, "uri is NULL.");
        return -1;
    }
    
    os_thread_mutex_lock(agent->lock);
    if (agent->state != kxplayer_agent_state_idle) {
        os_thread_mutex_unlock(agent->lock);
        OS_LOG(os_log_error, "agent state is not idle, can't open.");
        return -1;
    }

    agent->audio_stream = -1;
    agent->format_ctx = avformat_alloc_context();
    /* Open video file */
    int ret = avformat_open_input(&agent->format_ctx, uri, NULL, NULL);
    if(ret != 0) {
        OS_LOG(os_log_error, "error: %d, unable to open %s.", ret, uri);
        goto err1; // Couldn't open file
    }
    
    /* Retrieve stream information */
    if(avformat_find_stream_info(agent->format_ctx , NULL) < 0) {
        OS_LOG(os_log_error, "unable to find stream info.");
        goto err1; // Couldn't find stream information
    }
    
    /* Find the first video stream */
    int i = 0;
    int audioStream = -1;
    for(; i<agent->format_ctx->nb_streams; i++) {
        if(agent->format_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            audioStream=i;
        }
    }
    if(audioStream==-1) {
        OS_LOG(os_log_error, "no audio stream found.\n");
        goto err1;
    }
    agent->audio_stream = audioStream;
    
    AVCodecContext *aCodecCtx;
    AVCodec *aCodec;
    
    aCodecCtx = agent->format_ctx->streams[audioStream]->codec;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if(!aCodec) {
        OS_LOG(os_log_error, "Unsupported codec!\n");
        goto err1;
    }
    
    /* fire startup callback if any */
    if (context != NULL) {
        context->has_audio = 1;
        context->audio_sample_rate = aCodecCtx->sample_rate;
        context->audio_sample_format = translate_sample_format(aCodecCtx->sample_fmt);
        context->audio_channels = aCodecCtx->channels;
        
        context->has_video = 0; 
    }
    
    if (avcodec_open2(aCodecCtx, aCodec, NULL) != 0) {
        OS_LOG(os_log_error, "unable to open avcodec.\n");
        goto err1;
    }
    
    agent->state = kxplayer_agent_state_open;
    os_thread_mutex_unlock(agent->lock);
    return 0;
    
err1:
    if (agent->format_ctx != NULL) {
        avformat_free_context(agent->format_ctx);
        agent->format_ctx = NULL;
    }
    os_thread_mutex_unlock(agent->lock);
    return -1;
}

int kxplayer_agent_start(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, -1);
    os_thread_mutex_lock(agent->lock);
    if (agent->state != kxplayer_agent_state_open) {
        os_thread_mutex_unlock(agent->lock);
        OS_LOG(os_log_error, "unable to create lock.");
        return -1;
    }
    agent->state = kxplayer_agent_state_started;
    os_thread_mutex_unlock(agent->lock);

    AVCodecContext* aCodecCtx = agent->format_ctx->streams[agent->audio_stream]->codec;
    
    /* read and decode packet */
    AVPacket packet;
    AVFrame* frame = avcodec_alloc_frame();
    if (frame == NULL) {
        OS_LOG(os_log_error, "unable to alloc frame memory.");
        goto err1;
    }
    
    OS_LOG(os_log_debug, "start to read frame.");
    int frames = 0;
    while (1) {
        /* check if need to pause or abort */
        int state = kxplayer_agent_getstate(agent);
        if (state == kxplayer_agent_state_aborted) {
            OS_LOG(os_log_debug, "abort\n");
            break;
        } else if (state == kxplayer_agent_state_paused) {
            OS_ASSERT_DONT_STOP(os_thread_event_wait(agent->read_wait)==0);
            os_thread_mutex_lock(agent->lock);
            agent->state = kxplayer_agent_state_started;
            os_thread_mutex_unlock(agent->lock);
        }
        
        int ret = av_read_frame(agent->format_ctx, &packet); 
        if (ret < 0) {
            OS_LOG(os_log_debug, "failed to read frame.(%d)\n", ret);
            break;
        }
        avcodec_get_frame_defaults(frame);
        if (packet.stream_index == agent->audio_stream) {
            /* decode audio frame */
            if (avcodec_decode_audio4(aCodecCtx, frame, &frames, &packet) > 0) {
                while (frames > 0) {
                    int data_size = av_samples_get_buffer_size(NULL, 
                                                               aCodecCtx->channels,
                                                               frame->nb_samples,
                                                               aCodecCtx->sample_fmt,
                                                               1);
                    --frames;
                    if (agent->option.receive_callback != NULL) {
                        agent->option.receive_callback(frame->data[0], data_size, agent->option.userdata);
                    }
                
                    /* check if need to pause or abort */
                    int state = kxplayer_agent_getstate(agent);
                    if (state == kxplayer_agent_state_aborted) {
                        av_free_packet(&packet);
                        break;
                    } else if (state == kxplayer_agent_state_paused) {
                        OS_ASSERT_DONT_STOP(os_thread_event_wait(agent->read_wait)==0);
                        os_thread_mutex_lock(agent->lock);
                        agent->state = kxplayer_agent_state_started;
                        os_thread_mutex_unlock(agent->lock);
                    }
                }
            } else {
                OS_LOG(os_log_error, "error.\n");
                /* error occured */
            }
        }
        av_free_packet(&packet);
    }
    av_free(frame);
    avformat_close_input(&agent->format_ctx);
    agent->format_ctx = NULL;
    OS_LOG(os_log_debug, "stream closed.\n");

    os_thread_mutex_lock(agent->lock);
    agent->state = kxplayer_agent_state_idle;
    os_thread_mutex_unlock(agent->lock);

    if (agent->option.finish_callback != NULL) {
        agent->option.finish_callback(agent->option.userdata);
    }
    
    return 0;
    
err1:
    avformat_close_input(&agent->format_ctx);
    return -1;
}


int kxplayer_agent_pause(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, -1);
    OS_ASSERT(os_thread_mutex_lock(agent->lock)==0, -1);
    agent->state = kxplayer_agent_state_paused;
    OS_ASSERT(os_thread_mutex_unlock(agent->lock)==0, -1);
    return 0;
}

int kxplayer_agent_resume(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, -1);
    return os_thread_event_fire(agent->read_wait);
}

int kxplayer_agent_abort(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, -1);
    os_thread_mutex_lock(agent->lock);
    agent->state = kxplayer_agent_state_aborted;
    os_thread_mutex_unlock(agent->lock);
    os_thread_event_fire(agent->read_wait);
    return 0;
}

int kxplayer_agent_getstate(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, -1);
    os_thread_mutex_lock(agent->lock);
    int state = agent->state;
    os_thread_mutex_unlock(agent->lock);
    return state;
}











