
#include <osapi/core.h>

#if defined (OSAPI_ANDROID)

#include <jni.h>
#include <osapi/log.h>
#include "../android.h"

static void test(void) {
}

static JavaVM* g_jvm = NULL;

JavaVM* kxplayer_getjavavm(void) {
    return g_jvm;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    test();
    
    if (kxplayer_initialize() != 0) {
        OS_LOG(os_log_fatal,  "unable to initialize kxplayer library.");
        return JNI_ERR;
    }
    
    struct os_log_backend* backend = 
    os_log_backend_create(os_log_console_interface(), NULL);
    os_log_add_backend(backend);
    OS_LOG(os_log_debug, "osapi log initialized.");

    g_jvm = jvm;
    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *jvm, void *reserved) {
    kxplayer_terminate();
    g_jvm = NULL;
}

#endif
