
#include <osapi/core.h>

#if defined(OSAPI_ANDROID)

#include <osapi/memory.h>
#include <osapi/log.h>
#include <jni.h>

#include "boot.h"
#include "../api/kxplayer/agent.h"

struct agent_object {
    jobject start_listener;
    jobject receive_listener;
    jobject finish_listener;
    struct kxplayer_agent* agent;
};

static int start_callback(struct kxplayer_avcontext* tmpctx, void* userdata) {
    OS_LOG(os_log_debug, "start callback, agent_object:0x%0X, tmpctx:0x%0X.", userdata, tmpctx);
    struct agent_object* agobj = (struct agent_object*)userdata;
    if (agobj->start_listener == NULL) {
        return 0;
    }
    
    JavaVM* jvm = kxplayer_getjavavm();
    OS_CHECK(jvm!=NULL, ;);
    JNIEnv *env = NULL;
    if ((*jvm)->AttachCurrentThread(jvm, &env, NULL) < 0) {
        OS_LOG(os_log_error, "AttachCurrentThread failed.");
        return -1;
    }
    if (env == NULL) {
        OS_LOG(os_log_error, "unable to get JNIEnv.");
        return -1;
    }
    
    struct AVContext* ctx_class = kxplayer_lock_AVContext_class(); 
    jobject context = (*env)->NewObject(env, ctx_class->klass, ctx_class->constructor);
    if (context == NULL) {
        OS_LOG(os_log_error, "unable to create AVContext object.");
        goto err0;
    }

    (*env)->SetBooleanField(env, context, ctx_class->mHasAudio, tmpctx->has_audio);
    (*env)->SetIntField(env, context, ctx_class->mAudioSampleRate, tmpctx->audio_sample_rate);
    (*env)->SetIntField(env, context, ctx_class->mAudioSampleFormat, tmpctx->audio_sample_format);
    (*env)->SetIntField(env, context, ctx_class->mAudioChannels, tmpctx->audio_channels);
    (*env)->SetBooleanField(env, context, ctx_class->mHasVideo, tmpctx->has_video);
    
    /* call Agent.OnStartListener */
    struct AgentListener* listener = kxplayer_lock_AgentListener();
    OS_LOG(os_log_debug, "call start listener.");
    jint ret = (*env)->CallIntMethod(env, agobj->start_listener, listener->on_start, context);
    
    kxplayer_unlock_AgentListener();
    (*env)->DeleteLocalRef(env, context);
    kxplayer_unlock_AVContext_class();
    (*jvm)->DetachCurrentThread(jvm);
    return ret;
    
err0:
    kxplayer_unlock_AVContext_class();
    (*jvm)->DetachCurrentThread(jvm);
    return -1;
}

static void receive_callback(void* data, os_size size, void* ud) {
    OS_LOG(os_log_debug, "receive_callback, data:0x%0X, size:%lu.", data, size);
    
    JavaVM* jvm = kxplayer_getjavavm();
    OS_CHECK(jvm!=NULL, ;);
    JNIEnv *env = NULL;
    if ((*jvm)->AttachCurrentThread(jvm, &env, NULL) < 0) {
        OS_LOG(os_log_error, "AttachCurrentThread failed.");
        return;
    }
    if (env == NULL) {
        OS_LOG(os_log_error, "unable to get JNIEnv.");
        return;
    }
    
    struct agent_object* agobj = (struct agent_object*)ud;
    OS_LOG(os_log_debug, "agent object pointer in receive_callback 0x%0X.", agobj);
    
    if (agobj->receive_listener != NULL) {
        /* Create buffer for java */
        jbyteArray buffer = (*env)->NewByteArray(env, size);
        if (buffer == NULL) {
            OS_LOG(os_log_error, "failed to create buffer for OnReceiveListener.");
            return;
        }
        OS_LOG(os_log_debug, "create buffer array 0x%0X.", buffer);
        (*env)->SetByteArrayRegion(env, buffer, 0, size, (jbyte *)data);
        OS_LOG(os_log_debug, "copied buffer %lu bytes to byte array.", size);
        
        struct AgentListener* agent_listener = kxplayer_lock_AgentListener();
        (*env)->CallVoidMethod(env, agobj->receive_listener, agent_listener->on_receive, buffer);
        kxplayer_unlock_AgentListener();
        
        (*env)->DeleteLocalRef(env, buffer);
    }
    
    (*jvm)->DetachCurrentThread(jvm);
}

static void finish_callback(int status, void* userdata) {
    OS_LOG(os_log_debug, "finish_callback, status:%d.", status);
    
    JavaVM* jvm = kxplayer_getjavavm();
    OS_CHECK(jvm!=NULL, ;);
    JNIEnv *env = NULL;
    if ((*jvm)->AttachCurrentThread(jvm, &env, NULL) < 0) {
        OS_LOG(os_log_error, "AttachCurrentThread failed.");
        return;
    }
    if (env == NULL) {
        OS_LOG(os_log_error, "unable to get JNIEnv.");
        return;
    }
    
    struct agent_object* agobj = (struct agent_object*)userdata;
    OS_LOG(os_log_debug, "agent object pointer in finish_callback 0x%0X.", agobj);
    
    if (agobj->finish_listener != NULL) {
        struct AgentListener* agent_listener = kxplayer_lock_AgentListener();
        (*env)->CallVoidMethod(env, agobj->finish_listener, agent_listener->on_finish, status);
        kxplayer_unlock_AgentListener(); 
    }
    
    (*jvm)->DetachCurrentThread(jvm);
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    create
 * Signature: (Lcom/kaixindev/kxplayer/Agent/Option;)Lcom/kaixindev/kxplayer/Agent;
 */
JNIEXPORT jobject JNICALL Java_com_kaixindev_kxplayer_Agent_create
(JNIEnv *env, jclass klass, jobject on_start, jobject on_recv, jobject on_finish) {       
    struct agent_object* obj = 
    (struct agent_object*)os_calloc(sizeof(struct agent_object), 1);
    if (obj == NULL) {
        OS_LOG(os_log_error, "failed to create agent_object.");
        return NULL;
    }
    obj->agent = NULL;
    obj->start_listener = on_start==NULL ? NULL : (*env)->NewGlobalRef(env, on_start);
    obj->receive_listener = on_recv==NULL ? NULL : (*env)->NewGlobalRef(env, on_recv);
    obj->finish_listener = on_finish==NULL? NULL : (*env)->NewGlobalRef(env, on_finish);
    
    struct kxplayer_agent_option option;
    option.userdata = obj;
    option.start_callback = start_callback;
    option.receive_callback = receive_callback;
    option.finish_callback = finish_callback;
    obj->agent = kxplayer_agent_create(&option);
    if (obj->agent == NULL) {
        OS_LOG(os_log_error, "failed to create kxplayer agent.\n");
        goto err1;
    }
    OS_LOG(os_log_debug, "kxplayer agent pointer 0x%0X.", obj);
    
    jclass agent_class = (*env)->FindClass(env, "com/kaixindev/kxplayer/Agent");
    if (agent_class == NULL) {
        OS_LOG(os_log_error, "unable to find class com/kaixindev/kxplayer/Agent");
        goto err2;
    }
    jmethodID mid = (*env)->GetMethodID(env, agent_class, "<init>", "()V");
    jobject java_obj = (*env)->NewObject(env, agent_class, mid);
    if (java_obj == NULL) {
        OS_LOG(os_log_error, "unable to create com.kaixindev.kxplayer.Agent object.");
        goto err2;
    }
    
    jfieldID fid = (*env)->GetFieldID(env, agent_class, "mPtrNativeDev", "I");
    if (fid == NULL) {
        OS_LOG(os_log_error, "unable to get field mPtrNativeDev.");
        goto err3;
    }
    (*env)->SetIntField(env, java_obj, fid, (jint)obj);
    
    return java_obj;
    
err3:
    (*env)->DeleteLocalRef(env, java_obj);
err2:
    kxplayer_agent_release(obj->agent);
err1:
    if (obj->start_listener != NULL) {
        (*env)->DeleteGlobalRef(env, obj->start_listener);
    }
    if (obj->receive_listener != NULL) {
        (*env)->DeleteGlobalRef(env, obj->receive_listener);
    }
    if (obj->finish_listener != NULL) {
        (*env)->DeleteGlobalRef(env, obj->finish_listener);
    }
    os_free(obj);
    return NULL;
}

static struct agent_object* get_agent_ptr(JNIEnv* env, jobject self) {
    jclass agent_class = (*env)->GetObjectClass(env, self);
    if (agent_class == NULL) {
        return NULL;
    }
    jfieldID fid = (*env)->GetFieldID(env, agent_class, "mPtrNativeDev", "I");
    if (fid == NULL) {
        OS_LOG(os_log_error, "Field mPtrNativeDev not found.");
        return NULL;
    }
    (*env)->DeleteLocalRef(env, agent_class);
    
    jint ptr =  (*env)->GetIntField(env, self, fid);
    return (struct agent_object*)ptr;    
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    open
 * Signature: (Ljava/lang/String;Lcom/kaixindev/kxplayer/AVContext;)I
 */
JNIEXPORT jint JNICALL Java_com_kaixindev_kxplayer_Agent_open
(JNIEnv *env, jobject self, jstring uri) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        OS_LOG(os_log_error, "couldn't get agent object(%X).", agobj);
        return -1;
    }
    OS_LOG(os_log_debug, "agent object pointer 0x%0X.", agobj);
    
    const jbyte* struri = (*env)->GetStringUTFChars(env, uri, NULL);
    if (struri == NULL) {
        OS_LOG(os_log_error, "unable to get uri string.");
        return -1; /* OutOfMemoryError already thrown */
    }
    
    OS_LOG(os_log_debug, "open uri %s.", struri);
    int ret = kxplayer_agent_open(agobj->agent, struri);
    (*env)->ReleaseStringUTFChars(env, uri, struri);
    if (ret != 0) {
        OS_LOG(os_log_error, "unable to open uri.");
        return -1;
    }
    
    return 0;
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    pause
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_kaixindev_kxplayer_Agent_pause
(JNIEnv *env, jobject self) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        return -1;
    }
    return kxplayer_agent_pause(agobj->agent);
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    resume
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_kaixindev_kxplayer_Agent_resume
(JNIEnv *env, jobject self) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        return -1;
    }
    return kxplayer_agent_resume(agobj->agent);
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    abort
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_kaixindev_kxplayer_Agent_abort
(JNIEnv *env, jobject self) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        return -1;
    }
    return kxplayer_agent_abort(agobj->agent);
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    stop
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_kaixindev_kxplayer_Agent_stop
(JNIEnv *env, jobject self) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        return -1;
    }
    return kxplayer_agent_stop(agobj->agent);
}


/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    getState
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_kaixindev_kxplayer_Agent_getState
(JNIEnv *env, jobject self) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        return -1;
    }
    return kxplayer_agent_getstate(agobj->agent);
}


/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    release
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_kaixindev_kxplayer_Agent_release
(JNIEnv *env, jobject self) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        return;
    }
    if (agobj->receive_listener != NULL) {
        (*env)->DeleteGlobalRef(env, agobj->receive_listener);
        agobj->receive_listener = NULL;
    }
    if (agobj->finish_listener != NULL) {
        (*env)->DeleteGlobalRef(env, agobj->finish_listener);
        agobj->finish_listener = NULL;
    }
    if (agobj->start_listener != NULL) {
        (*env)->DeleteGlobalRef(env, agobj->start_listener);
        agobj->start_listener = NULL;
    }
    kxplayer_agent_release(agobj->agent);
    os_free(agobj);
}

#endif
























