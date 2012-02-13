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

#include <osapi/thread.h>
static struct os_thread_event* g_wait = NULL;

int main(int argc, char *argv[])
{
    g_wait = os_thread_event_create();
    
    if (kxplayer_initialize() != 0) {
        fprintf(stderr, "failed to initialize kaixin player.\n");
        return -1;
    }
     
    if (kxplayer_play(HTTP_WMA) != 0) {
        fprintf(stderr, "unable to play %s\n", HTTP_WMA);
        goto err1;
    }
    os_thread_event_wait(g_wait);
    
    os_thread_event_release(g_wait);
    kxplayer_terminate();
    fprintf(stdout, "Player quit.");
    return 0;
    
err1:
    kxplayer_terminate();
}













