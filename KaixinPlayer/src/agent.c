
#include <osapi/buffer.h>
#include <osapi/thread.h>
#include <osapi/memory.h>
#include <osapi/log.h>

#include <libavformat/avformat.h>
#include <libavutil/error.h>

#include "api/kxplayer/agent.h"
#include "api/kxplayer/device.h"

#define SRC_PLAYER_JOB_COUNT    12

struct _agent_job {
    char* uri;
};

struct kxplayer_agent {
    struct kxplayer_agent_option option;
    struct os_thread_event* read_wait;
    struct os_thread_mutex* lock;
    os_int32 state;
    
    struct os_thread_event* quit_event;
    int quit;
    
    struct os_thread* worker;
    struct os_ringbuf* jobs;
};

static int translate_sample_format(int format) {
    switch (format) {
        case AV_SAMPLE_FMT_S16:
        default:
            return kxplayer_audio_sample_format_s16;
    }
}

static os_intptr agent_threadproc(void* data) {
    struct kxplayer_agent* agent = (struct kxplayer_agent*)data;
    struct kxplayer_agent_option* option = &agent->option;
    
    OS_LOG(os_log_debug, "agent thread started.");
    
    while (1) {
    start:
        os_thread_mutex_lock(agent->lock);
        if (agent->quit) {
            os_thread_mutex_unlock(agent->lock);
            break;
        }
        os_thread_mutex_unlock(agent->lock);
        
        os_thread_event_wait(agent->read_wait);
        
        os_thread_mutex_lock(agent->lock);
        if (agent->quit) {
            os_thread_mutex_unlock(agent->lock);
            break;
        }
        os_thread_mutex_unlock(agent->lock);
        
        /* ---------------------------------------------------------------------
         * open stream 
         */
        
        int status = kxplayer_agent_status_ok;
        
        OS_LOG(os_log_debug, "open stream");
        os_thread_mutex_lock(agent->lock);
        if (os_ringbuf_isempty(agent->jobs)) {
            os_thread_mutex_unlock(agent->lock);
            continue;
        }
        struct _agent_job job;
        os_ringbuf_pop(agent->jobs, &job);
        os_thread_mutex_unlock(agent->lock);
        
        OS_ASSERT(job.uri!=NULL, ;);
           
        AVFormatContext* format_ctx = avformat_alloc_context();
        /* Open video file */
        int ret = avformat_open_input(&format_ctx, job.uri, NULL, NULL);
        if(ret != 0) {
            OS_LOG(os_log_error, "error: %d, unable to open %s.", ret, job.uri);
            os_free(job.uri);
            job.uri = NULL;
            status = kxplayer_agent_status_error;
            goto err0;
        }
        
        /* Retrieve stream information */
        if(avformat_find_stream_info(format_ctx , NULL) < 0) {
            OS_LOG(os_log_error, "unable to find stream info.");
            status = kxplayer_agent_status_error;
            goto err1; // Couldn't find stream information
        }
        
        /* Find the first video stream */
        int i = 0;
        int audio_stream = -1;
        for(; i<format_ctx->nb_streams; i++) {
            if(format_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
                audio_stream = i;
            }
        }
        if(audio_stream==-1) {
            OS_LOG(os_log_error, "no audio stream found.\n");
            status = kxplayer_agent_status_error;
            goto err1;
        }
        
        AVCodecContext *aCodecCtx;
        AVCodec *aCodec;
        
        aCodecCtx = format_ctx->streams[audio_stream]->codec;
        aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
        if(!aCodec) {
            OS_LOG(os_log_error, "Unsupported codec!\n");
            status = kxplayer_agent_status_error;
            goto err1;
        }
        
        struct kxplayer_avcontext context;
        /* fire startup callback if any */
        context.has_audio = 1;
        context.audio_sample_rate = aCodecCtx->sample_rate;
        context.audio_sample_format = translate_sample_format(aCodecCtx->sample_fmt);
        context.audio_channels = aCodecCtx->channels;
        context.has_video = 0;
        if (option->start_callback != NULL && option->start_callback(&context, option->userdata)!=0) {
            status = kxplayer_agent_status_error;
            goto err1;
        }
        
        if (avcodec_open2(aCodecCtx, aCodec, NULL) != 0) {
            OS_LOG(os_log_error, "unable to open avcodec.\n");
            status = kxplayer_agent_status_error;
            goto err1;
        }
        
        os_thread_mutex_lock(agent->lock);
        agent->state = kxplayer_agent_state_open;
        os_thread_mutex_unlock(agent->lock);
        
        /* ---------------------------------------------------------------------
         * read stream 
         */
        
        OS_LOG(os_log_debug, "read stream");
        /* read and decode packet */
        AVPacket packet;
        AVFrame* frame = avcodec_alloc_frame();
        if (frame == NULL) {
            OS_LOG(os_log_error, "unable to alloc frame memory.");
            status = kxplayer_agent_status_error;
            goto err1;
        }
        
        OS_LOG(os_log_debug, "start to read frame.");
        int frames = 0;
        while (1) {
            /* check if need to pause or abort */
            int state = kxplayer_agent_getstate(agent);
            if (state == kxplayer_agent_state_aborted) {
                OS_LOG(os_log_debug, "abort\n");
                status = kxplayer_agent_status_aborted;
                break;
            } else if (state == kxplayer_agent_state_paused) {
                OS_ASSERT_DONT_STOP(os_thread_event_wait(agent->read_wait)==0);
                os_thread_mutex_lock(agent->lock);
                agent->state = kxplayer_agent_state_started;
                os_thread_mutex_unlock(agent->lock);
            }
            
            int ret = av_read_frame(format_ctx, &packet); 
            if (ret < 0) {
                OS_LOG(os_log_debug, "failed to read frame.(%d)\n", ret);
                status = (ret == AVERROR_EOF) ? kxplayer_agent_status_ok : kxplayer_agent_status_error;
                goto err1;
            }
            avcodec_get_frame_defaults(frame);
            if (packet.stream_index == audio_stream) {
                /* decode audio frame */
                if (avcodec_decode_audio4(aCodecCtx, frame, &frames, &packet) > 0) {
                    while (frames > 0) {
                        int data_size = av_samples_get_buffer_size(NULL, 
                                                                   aCodecCtx->channels,
                                                                   frame->nb_samples,
                                                                   aCodecCtx->sample_fmt,
                                                                   1);
                        --frames;
                        if (option->receive_callback != NULL) {
                            option->receive_callback(frame->data[0], data_size, option->userdata);
                        }
                        
                        /* check if need to pause or abort */
                        int state = kxplayer_agent_getstate(agent);
                        if (state == kxplayer_agent_state_aborted) {
                            OS_LOG(os_log_info, "aborted");
                            av_free_packet(&packet);
                            status = kxplayer_agent_status_aborted;
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
                    status = kxplayer_agent_status_error;
                    /* status = kxplayer_agent_status_error; */
                    /* error occured */
                }
            }
            av_free_packet(&packet);
        }
        av_free(frame);
        avformat_close_input(&format_ctx);
        format_ctx = NULL;
        OS_LOG(os_log_debug, "stream closed.\n");
        
        os_thread_mutex_lock(agent->lock);
        agent->state = kxplayer_agent_state_idle;
        os_thread_mutex_unlock(agent->lock);
        
        if (option->finish_callback != NULL) {
            option->finish_callback(status, option->userdata);
        }
        
        goto start;
    err1:
        avformat_close_input(&format_ctx);
        format_ctx = NULL;
    err0:
        if (option->finish_callback != NULL) {
            option->finish_callback(status, option->userdata);
        }
    }
    
    return 0;
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
    
    if ((agent->worker = os_thread_create("player worker")) == NULL) {
        OS_LOG(os_log_error, "failed to create thread.\n");
        goto err3;
    }
    
    if (os_thread_start(agent->worker, agent_threadproc, agent) != 0) {
        OS_LOG(os_log_error, "failed to start thread.\n");
        goto err4;
    }
    
    if ((agent->quit_event = os_thread_event_create()) == NULL) {
        OS_LOG(os_log_error, "failed to create quit event.\n");
        goto err4;
    }
    
    agent->jobs = os_ringbuf_create(sizeof(struct _agent_job), SRC_PLAYER_JOB_COUNT);
    if (agent->jobs == NULL) {
        OS_LOG(os_log_error, "failed to create jobs queue.\n");
        goto err5;
    }
    
    agent->quit = 0;
    
    return agent;
 
err6:
    os_ringbuf_release(agent->jobs);
err5:
    os_thread_event_release(agent->quit_event);
err4:
    os_thread_release(agent->worker);
err3:
    os_thread_mutex_release(agent->lock);
err2:
    os_thread_event_release(agent->read_wait);
err1:
    os_free(agent);
    return NULL;
}


void kxplayer_agent_release(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, ;);
    
    kxplayer_agent_stop(agent);
    os_thread_release(agent->worker);
    
    if (agent->quit_event != NULL) {
        os_thread_event_release(agent->quit_event);
        agent->quit_event = NULL;
    }
    
    if (agent->read_wait != NULL) {
        os_thread_event_release(agent->read_wait);
        agent->read_wait = NULL;
    }
    
    if (agent->lock != NULL) {
        os_thread_mutex_release(agent->lock);
        agent->lock = NULL;
    }
    
    if (agent->jobs != NULL) {
        os_ringbuf_release(agent->jobs);
        agent->jobs = NULL;
    }
}

int kxplayer_agent_open(struct kxplayer_agent* agent, const char* uri) {
    OS_ASSERT(agent!=NULL, -1);
    OS_ASSERT(uri!=NULL, -1);
    
    int state = kxplayer_agent_getstate(agent);
    if (state != kxplayer_agent_state_idle) {
        kxplayer_agent_abort(agent);
    }
    
    struct _agent_job job;
    if ((job.uri = os_malloc(strlen(uri)+1)) == NULL) {
        return -1;
    }
    strcpy(job.uri, uri);
    os_thread_mutex_lock(agent->lock);
    if (os_ringbuf_push(agent->jobs, &job) != 0) {
        os_thread_mutex_unlock(agent->lock);
        return -1;
    }
    os_thread_mutex_unlock(agent->lock);
    
    os_thread_event_fire(agent->read_wait);
    return 0;
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

int kxplayer_agent_stop(struct kxplayer_agent* agent) {
    OS_ASSERT(agent!=NULL, -1);
    kxplayer_agent_abort(agent);
    os_thread_mutex_lock(agent->lock);
    agent->quit = 1;
    os_thread_mutex_unlock(agent->lock);
    os_thread_event_fire(agent->quit_event);
    return 0;
}











