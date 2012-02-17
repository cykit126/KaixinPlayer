
#ifndef KaixinPlayer_boot_h
#define KaixinPlayer_boot_h

#include <jni.h>

extern JavaVM* kxplayer_getjavavm(void);

struct AVContext {
    jclass klass;
    jmethodID constructor;
    jfieldID mHasAudio;
    jfieldID mAudioSampleRate;
    jfieldID mAudioSampleFormat;
    jfieldID mAudioChannels;
    jfieldID mHasVideo;
    struct os_thread_mutex* lock;
};
extern struct AVContext* kxplayer_lock_AVContext_class(void);
extern void kxplayer_unlock_AVContext_class(void);

struct AgentListener {
    jclass start_listener;
    jmethodID on_start;
    
    jclass receive_listener;
    jmethodID on_receive;
    
    jclass finish_listener;
    jmethodID on_finish;
    
    struct os_thread_mutex* lock;
};

extern struct AgentListener* kxplayer_lock_AgentListener(void);
extern void kxplayer_unlock_AgentListener(void);


#endif








