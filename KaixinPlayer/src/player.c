
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

int kxplayer_initialize(void) {
    if (osapi_initialize() != 0) {
        fprintf(stderr, "failed to initialize osapi.\n");
        return -1;
    }
    
    if (internal_device_initialize() != 0) {
        fprintf(stderr, "failed to initialize device.\n");
        goto err1;
    }
    
    av_register_all();
    avformat_network_init();
    
    return 0;
    
err2:
    internal_device_terminate();
err1:
    osapi_terminate();
    return -1;
}

void kxplayer_terminate(void) {
    avformat_network_deinit();
    internal_device_terminate();
    osapi_terminate();
}








