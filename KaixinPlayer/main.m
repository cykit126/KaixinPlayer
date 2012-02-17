//
//  main.m
//  KaixinPlayer
//
//  Created by Wilbur on 2/1/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "src/api/kxplayer/player.h"
#include "src/api/kxplayer/agent.h"
#include "src/api/kxplayer/device.h"

#define MP3_FILE    "/Users/kaixin/Work/RadioPlayer/KaixinPlayer/xiarixing.wma"
#define RTSP_URL    "rtsp://a17.l211053182.c2110.a.lm.akamaistream.net/D/17/2110/v0001/reflector:53182"
#define RTSP_URL2   "rtsp://211.100.39.175/mopradio"
#define HTTP_URL    "http://wmlive.bbc.net.uk/wms%5Cbbc_ami%5Cradio2%5Cradio2_bb_live_int_ep1_sw0"
#define HTTP_MP3    "http://zhangmenshiting2.baidu.com/data2/music/13729339/13729339.mp3?xcode=7145d876f0cf755b7773641246fea1a0&mid=0.08626986349644"
#define HTTP_WMA    "http://www.liming2009.com/108/music/ruguonishiwodechuanshuo.wma"

#include <osapi/log.h>
#include <osapi/thread.h>
#include <osapi/system.h>

static int init(void) {
    
    return 0;
}

static int start(struct kxplayer_avcontext* context, void* userdata) {
    OS_LOG(os_log_debug, "start callback.");
    return 0;
}

static void receive(void* data, os_size size, void* userdata) {
    OS_LOG(os_log_debug, "receive data.");
}

int main(int argc, char *argv[])
{
    if (kxplayer_initialize() != 0) {
        OS_LOG(os_log_error, "unable to initialize kxplayer.");
        return -1;
    }
    os_log_setlevel(os_log_trace);
    struct os_log_backend* console = os_log_backend_create(os_log_console_interface(), NULL);
    os_log_add_backend(console);

    struct kxplayer_agent_option option;
    option.start_callback = start;
    option.receive_callback = receive;
    option.finish_callback = NULL;
    option.userdata = NULL;
    struct kxplayer_agent* agent = kxplayer_agent_create(&option);
    if (agent == NULL) {
        OS_LOG(os_log_error, "unable to create agent.");
        return -1;
    }
    
    if (kxplayer_agent_open(agent, RTSP_URL) != 0) {
        OS_LOG(os_log_error, "unable to open uri %s.", RTSP_URL);
        return -1;
    }
    
    os_sleep(5000);
    kxplayer_agent_open(agent, MP3_FILE);
    os_sleep(999999);
    
    kxplayer_agent_stop(agent);
    
    kxplayer_terminate();
    
    return 0;
}













