
#include <osapi/core.h>

#if defined(OSAPI_ANDROID)

#include <osapi/memory.h>
#include <osapi/log.h>
#include <jni.h>

#include "boot.h"
#include "../api/kxplayer/agent.h"

struct agent_object {
    jobject receive_listener;
    jobject finish_listener;
    struct kxplayer_agent* agent;
};

static void receive_callback(void* data, os_size size, void* ud) {
    OS_LOG(os_log_debug, "receive_callback, data:0x%0X, size:%lu.", data, size);
    
    JavaVM* jvm = kxplayer_getjavavm();
    OS_CHECK(jvm!=NULL, ;);
    JNIEnv *env = NULL;
    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_2);
    
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
        
        jclass listener_class = (*env)->GetObjectClass(env, agobj->receive_listener);
        jmethodID mid = (*env)->GetMethodID(env,
                                            listener_class,
                                            "onReceive",
                                            "([B)V");
        if (mid == NULL) {
            OS_LOG(os_log_error, "failed to find onReceive method.");
            (*env)->DeleteLocalRef(env, listener_class);
            (*env)->DeleteLocalRef(env, buffer);
            return;
        }
        OS_LOG(os_log_debug, "get method onReceive of OnReceiveListener.");
        (*env)->CallVoidMethod(env, agobj->receive_listener, mid, buffer);
        OS_LOG(os_log_debug, "called method onReceive of OnReceiveListener.");
        
        (*env)->DeleteLocalRef(env, listener_class);
        (*env)->DeleteLocalRef(env, buffer);
    }
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    create
 * Signature: (Lcom/kaixindev/kxplayer/Agent/Option;)Lcom/kaixindev/kxplayer/Agent;
 */
JNIEXPORT jobject JNICALL Java_com_kaixindev_kxplayer_Agent_create
(JNIEnv *env, jclass klass) {       
    struct agent_object* obj = 
        (struct agent_object*)os_calloc(sizeof(struct agent_object), 1);
    if (obj == NULL) {
        OS_LOG(os_log_error, "failed to create agent_object.");
        return NULL;
    }
    obj->agent = NULL;
    obj->receive_listener = NULL;
    obj->finish_listener = NULL;
    
    struct kxplayer_agent_option option;
    option.userdata = obj;
    option.receive_callback = receive_callback;
    option.finish_callback = NULL;
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
(JNIEnv *env, jobject self, jstring uri, jobject context) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        OS_LOG(os_log_error, "couldn't get agent object(%X).", agobj);
        return -1;
    }
    OS_LOG(os_log_error, "agent object pointer 0x%0X.", agobj);
    
    const jbyte* struri = (*env)->GetStringUTFChars(env, uri, NULL);
    if (struri == NULL) {
        OS_LOG(os_log_error, "unable to get uri string.");
        return -1; /* OutOfMemoryError already thrown */
    }
    
    OS_LOG(os_log_info, "open uri %s.", struri);
    struct kxplayer_avcontext tmpctx;
    int ret = kxplayer_agent_open(agobj->agent, struri, &tmpctx);
    (*env)->ReleaseStringUTFChars(env, uri, struri);
    if (ret != 0) {
        OS_LOG(os_log_error, "unable to open uri.");
        return -1;
    }
    
    /* set AVContext */
    jclass ctx_class = (*env)->GetObjectClass(env, context);
    if (ctx_class == NULL) {
        OS_LOG(os_log_error, "unable to get AVContext class.");
        return -1;
    }
    
    jfieldID fid = (*env)->GetFieldID(env, ctx_class, "mHasAudio", "Z");
    if (fid == NULL) {
        OS_LOG(os_log_error, "Field mHasAudio not found.");
        return -1;
    }
    (*env)->SetBooleanField(env, context, fid, tmpctx.has_audio);
    
    fid = (*env)->GetFieldID(env, ctx_class, "mAudioSampleRate", "I");
    if (fid == NULL) {
        OS_LOG(os_log_error, "Field mAudioSampleRate not found.");
    } 
    (*env)->SetIntField(env, context, fid, tmpctx.audio_sample_rate);
    
    fid = (*env)->GetFieldID(env, ctx_class, "mAudioSampleFormat", "I");
    if (fid == NULL) {
        OS_LOG(os_log_error, "Field mAudioSampleFormat not found.");
    } 
    (*env)->SetIntField(env, context, fid, tmpctx.audio_sample_format);

    fid = (*env)->GetFieldID(env, ctx_class, "mAudioChannels", "I");
    if (fid == NULL) {
        OS_LOG(os_log_error, "Field mAudioChannels not found.");
    } 
    (*env)->SetIntField(env, context, fid, tmpctx.audio_channels);

    fid = (*env)->GetFieldID(env, ctx_class, "mHasVideo", "Z");
    if (fid == NULL) {
        OS_LOG(os_log_error, "Field mHasVideo not found.");
    } 
    (*env)->SetBooleanField(env, context, fid, tmpctx.has_video);
 
    return 0;
}

/*
 * Class:     com_kaixindev_kxplayer_Agent
 * Method:    start
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_kaixindev_kxplayer_Agent_start
(JNIEnv *env, jobject self, jobject receive_listener, jobject finish_listener) {
    struct agent_object* agobj = get_agent_ptr(env, self);
    if (agobj == NULL) {
        OS_LOG(os_log_error, "agent_object is NULL.");
        return -1;
    }
    agobj->receive_listener = (*env)->NewGlobalRef(env, receive_listener);
    agobj->finish_listener = (*env)->NewGlobalRef(env, finish_listener);
    return kxplayer_agent_start(agobj->agent);
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
    kxplayer_agent_release(agobj->agent);
    os_free(agobj);
}

#endif
























