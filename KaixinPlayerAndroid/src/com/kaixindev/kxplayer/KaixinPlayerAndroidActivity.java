package com.kaixindev.kxplayer;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

public class KaixinPlayerAndroidActivity extends Activity {
	
	public static final String LOGTAG = "console";
	
	static {
		System.loadLibrary("kxplayer");
	}
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        //String uri = "/data/data/com.kaixindev.kxplayer/app_appdata/legend.mp3";
        String uri = "rtsp://a17.l211053182.c2110.a.lm.akamaistream.net/D/17/2110/v0001/reflector:53182";
        //String uri = "http://www.liming2009.com/108/music/ruguonishiwodechuanshuo.wma";
        Intent intent = new Intent(PlayerService.START_PLAYER);
        intent.putExtra(PlayerService.PROPERTY_URI, uri);
        startService(intent);
    }
}