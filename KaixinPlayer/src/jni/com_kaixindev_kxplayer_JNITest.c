
#include <osapi/core.h>

#ifdef OSAPI_ANDROID

#include "com_kaixindev_kxplayer_JNITest.h"
#include <sys/socket.h>
#include <netdb.h>
#include <android/log.h>

JNIEXPORT void JNICALL Java_com_kaixindev_kxplayer_JNITest_test
(JNIEnv *env, jclass klass) {
    struct addrinfo hints, *info, *cur;
    int error;
    struct hostent *hostent;
    
    const char host[] = "qq.com";
    const char port[] = "80";
    
    char *old_mode, *old_mode_dup;
    
    memset(&hints, 0, sizeof(hints));
    
    hints.ai_family = AF_INET;
    
    __android_log_print(ANDROID_LOG_INFO, "ResolvTest",
                        "about to call getaddrinfo(%s,%s)", host, port);
    
    error = getaddrinfo(host, port, &hints, &info);
    
    if(error != 0)
    {
        __android_log_print(ANDROID_LOG_WARN, "ResolvTest",
                            "getaddrinfo(%s,%s) failed: %d,%s", host, port, error, gai_strerror(error));
    }
    else {
        __android_log_print(ANDROID_LOG_INFO, "ResolvTest",
                            "getaddrinfo(%s,%s) succeeded", host, port);
        freeaddrinfo(info);
    }
    
    __android_log_print(ANDROID_LOG_INFO, "ResolvTest",
                        "about to call gethostbyname(%s)", host);
    
    hostent = gethostbyname(host);
    
    if (hostent == NULL) {
        __android_log_print(ANDROID_LOG_WARN, "ResolvTest",
                            "gethostbyname(%s) failed: %d, %s", host, h_errno, hstrerror(h_errno));
    }
    else {
        __android_log_print(ANDROID_LOG_WARN, "ResolvTest",
                            "gethostbyname(%s) succeeded", host);
    }
}



#endif


















