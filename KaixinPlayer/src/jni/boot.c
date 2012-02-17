
#include <osapi/core.h>

#if defined (OSAPI_ANDROID)

#include <jni.h>
#include <osapi/log.h>
#include <osapi/thread.h>
#include "../android.h"
#include "boot.h"

static JavaVM* g_jvm = NULL;

JavaVM* kxplayer_getjavavm(void) {
    return g_jvm;
}

/* AVContext start */

static struct AVContext g_AVContext_class; 

struct AVContext* kxplayer_lock_AVContext_class(void) {
    os_thread_mutex_lock(g_AVContext_class.lock);
    return &g_AVContext_class;
}

void kxplayer_unlock_AVContext_class(void) {
    os_thread_mutex_unlock(g_AVContext_class.lock);
}

static int AVContext_init(JNIEnv* env) {
    g_AVContext_class.lock = os_thread_mutex_create();
    jclass ctx_class = (*env)->FindClass(env, "com/kaixindev/kxplayer/AVContext");
    if (ctx_class == NULL) {
        OS_LOG(os_log_error, "unable to get AVContext class.");
        goto err1;
    }
    g_AVContext_class.klass = (*env)->NewGlobalRef(env, ctx_class);
    
    g_AVContext_class.constructor = (*env)->GetMethodID(env, g_AVContext_class.klass, "<init>", "()V");
    if (g_AVContext_class.constructor == NULL) {
        OS_LOG(os_log_error, "default constructor of com.kaixindev.kxplayer.AVContext not found.");
        goto err2;
    }
    
    OS_LOG(os_log_debug, "create AVContext object.");
    g_AVContext_class.mHasAudio = (*env)->GetFieldID(env, ctx_class, "mHasAudio", "Z");
    if (g_AVContext_class.mHasAudio == NULL) {
        OS_LOG(os_log_error, "Field mHasAudio not found.");
        goto err2;
    }
    
    g_AVContext_class.mAudioSampleRate = (*env)->GetFieldID(env, ctx_class, "mAudioSampleRate", "I");
    if (g_AVContext_class.mAudioSampleRate == NULL) {
        OS_LOG(os_log_error, "Field mAudioSampleRate not found.");
        goto err2;
    } 
    
    g_AVContext_class.mAudioSampleFormat = (*env)->GetFieldID(env, ctx_class, "mAudioSampleFormat", "I");
    if (g_AVContext_class.mAudioSampleFormat == NULL) {
        OS_LOG(os_log_error, "Field mAudioSampleFormat not found.");
        goto err2;
    } 
    
    g_AVContext_class.mAudioChannels = (*env)->GetFieldID(env, ctx_class, "mAudioChannels", "I");
    if (g_AVContext_class.mAudioChannels == NULL) {
        OS_LOG(os_log_error, "Field mAudioChannels not found.");
        goto err2;
    } 
    
    g_AVContext_class.mHasVideo = (*env)->GetFieldID(env, ctx_class, "mHasVideo", "Z");
    if (g_AVContext_class.mHasVideo == NULL) {
        OS_LOG(os_log_error, "Field mHasVideo not found.");
        goto err2;
    }     
    
    OS_LOG(os_log_debug, "AVContext class found.");
    (*env)->DeleteLocalRef(env, ctx_class);
    return 0;
    
err2:
    (*env)->DeleteGlobalRef(env, g_AVContext_class.klass);
    g_AVContext_class.klass = NULL;
    (*env)->DeleteLocalRef(env, ctx_class);
err1:
    os_thread_mutex_release(g_AVContext_class.lock);
    g_AVContext_class.lock = NULL;
    return -1;
}

static void AVContext_deinit(JNIEnv* env) {
    if (g_AVContext_class.lock != NULL) {
        os_thread_mutex_release(g_AVContext_class.lock);
        g_AVContext_class.lock = NULL;
    }
    
    if (g_AVContext_class.klass != NULL) {
        (*env)->DeleteGlobalRef(env, g_AVContext_class.klass);
        g_AVContext_class.klass = NULL;
    }    
}

/* Agent listeners */
static struct AgentListener g_AgentListener;

static int AgentListener_init(JNIEnv* env) {
    jclass start_class = (*env)->FindClass(env, "com/kaixindev/kxplayer/Agent$OnStartListener");
    if (start_class == NULL) {
        OS_LOG(os_log_error, "class Agent.OnStartListener not found.");
        return -1;
    }
    g_AgentListener.start_listener = (*env)->NewGlobalRef(env, start_class);
    g_AgentListener.on_start = (*env)->GetMethodID(env, start_class, "onStart", "(Lcom/kaixindev/kxplayer/AVContext;)I");
    if (g_AgentListener.on_start == NULL) {
        OS_LOG(os_log_error, "Agent.OnStartListener.onStart not found.");
        goto err1;
    }
    
    jclass recv_class = (*env)->FindClass(env, "com/kaixindev/kxplayer/Agent$OnReceiveListener");
    if (recv_class == NULL) {
        OS_LOG(os_log_error, "class Agent.OnReceiveListener not found.");
        goto err1;
    }
    g_AgentListener.receive_listener = (*env)->NewGlobalRef(env, recv_class);
    g_AgentListener.on_receive = (*env)->GetMethodID(env, recv_class, "onReceive", "([B)V");
    if (g_AgentListener.on_receive == NULL) {
        OS_LOG(os_log_error, "Agent.OnReceiveListener.onReceive not found.");
        goto err2;
    }
    
    jclass finish_class = (*env)->FindClass(env, "com/kaixindev/kxplayer/Agent$OnFinishListener");
    if (finish_class == NULL) {
        OS_LOG(os_log_error, "class Agent.OnFinishListener not found.");
        goto err2;
    }
    g_AgentListener.finish_listener = (*env)->NewGlobalRef(env, finish_class);
    g_AgentListener.on_finish = (*env)->GetMethodID(env, finish_class, "onFinish", "()V");
    if (g_AgentListener.on_finish == NULL) {
        OS_LOG(os_log_error, "Agent.OnFinishListener.onFinish not found.");
        goto err3;
    }
    
    g_AgentListener.lock = os_thread_mutex_create();
    
    (*env)->DeleteLocalRef(env, finish_class);
    (*env)->DeleteLocalRef(env, recv_class);
    (*env)->DeleteLocalRef(env, start_class);
    return 0;
    
err3:
    (*env)->DeleteLocalRef(env, finish_class);
    (*env)->DeleteGlobalRef(env, g_AgentListener.finish_listener);
err2:
    (*env)->DeleteLocalRef(env, recv_class);
    (*env)->DeleteGlobalRef(env, g_AgentListener.receive_listener);
err1:
    (*env)->DeleteGlobalRef(env, g_AgentListener.start_listener);
    (*env)->DeleteLocalRef(env, start_class);
    return -1;
}

void AgentListener_deinit(JNIEnv* env) {
    if (g_AgentListener.start_listener != NULL) {
        (*env)->DeleteGlobalRef(env, g_AgentListener.start_listener);
        g_AgentListener.start_listener = NULL;
    }
    
    if (g_AgentListener.receive_listener != NULL) {
        (*env)->DeleteGlobalRef(env, g_AgentListener.receive_listener);
        g_AgentListener.receive_listener = NULL;
    }
    
    if (g_AgentListener.finish_listener != NULL) {
        (*env)->DeleteGlobalRef(env, g_AgentListener.finish_listener);
        g_AgentListener.finish_listener = NULL;
    }
    
    if (g_AgentListener.lock != NULL) {
        os_thread_mutex_release(g_AgentListener.lock);
        g_AgentListener.lock = NULL;
    }
}

struct AgentListener* kxplayer_lock_AgentListener(void) {
    os_thread_mutex_lock(g_AgentListener.lock);
    return &g_AgentListener;
}

void kxplayer_unlock_AgentListener(void) {
    os_thread_mutex_unlock(g_AgentListener.lock);
}


/* AVContext end */

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    g_AVContext_class.klass = NULL;
    g_AVContext_class.lock = NULL;
    
    if (kxplayer_initialize() != 0) {
        OS_LOG(os_log_fatal,  "unable to initialize kxplayer library.");
        return JNI_ERR;
    }
    
    os_log_setlevel(os_log_trace);
    struct os_log_backend* backend = 
    os_log_backend_create(os_log_console_interface(), NULL);
    os_log_add_backend(backend);
    OS_LOG(os_log_trace, "osapi log initialized.");
    
    g_jvm = jvm;
    
    JNIEnv *env = NULL;
    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_2);
    OS_ASSERT(env!=NULL, JNI_ERR);
    
    /* initilaize global refs. */
    if (AVContext_init(env) != 0) {
        return JNI_ERR;
    }
    
    if (AgentListener_init(env) != 0) {
        AVContext_deinit(env);
        return JNI_ERR;
    }
        
    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *jvm, void *reserved) {
    kxplayer_terminate();
    g_jvm = NULL;
    
    JNIEnv *env = NULL;
    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_2);
    OS_ASSERT(env!=NULL, ;);
    
    AVContext_deinit(env);
    AgentListener_deinit(env);
}

#endif
