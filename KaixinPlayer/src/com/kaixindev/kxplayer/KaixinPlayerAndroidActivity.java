package com.kaixindev.kxplayer;

import android.app.TabActivity;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TabHost;

public class KaixinPlayerAndroidActivity extends TabActivity {
	
	public static final String LOGTAG = "console";
	public static final String UPDATE_PLAYER_CONTROL = "com.kaixindev.kxplayer.UPDATE_PLAYER_CONTROL";
	
	static {
		System.loadLibrary("kxplayer");
	}
	
	private PlayerControlBroadcastrReceiver mPCBReceiver;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        mPCBReceiver = new PlayerControlBroadcastrReceiver(this);
        
        Resources res = getResources(); // Resource object to get Drawables
        TabHost tabHost = getTabHost();  // The activity TabHost
        TabHost.TabSpec spec;  // Resusable TabSpec for each tab
        Intent intent;  // Reusable Intent for each tab

        // Create an Intent to launch an Activity for the tab (to be reused)
        intent = new Intent().setClass(this, RecommActivity.class);

        // Initialize a TabSpec for each tab and add it to the TabHost
        spec = tabHost.newTabSpec("recomm")
        		.setIndicator(res.getString(R.string.tab_recomm), null)
                .setContent(intent);
        tabHost.addTab(spec);

        // Do the same for the other tabs
        intent = new Intent().setClass(this, CategoriesActivity.class);
        spec = tabHost.newTabSpec("categories")
        		.setIndicator(res.getString(R.string.tab_categories), null)
                .setContent(intent);
        tabHost.addTab(spec);

        intent = new Intent().setClass(this, FavoritesActivity.class);
        spec = tabHost.newTabSpec("favorites")
        		.setIndicator(res.getString(R.string.tab_favorites), null)
                .setContent(intent);
        tabHost.addTab(spec);
        
        intent = new Intent().setClass(this, SettingsActivity.class);
        spec = tabHost.newTabSpec("settings")
        		.setIndicator(res.getString(R.string.tab_settings), null)
                .setContent(intent);
        tabHost.addTab(spec);

        tabHost.setCurrentTab(0);        
        
        
        final Button btnStart = (Button)findViewById(R.id.player_control_start);
        btnStart.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				switch (Player.getInstance(KaixinPlayerAndroidActivity.this).getState()) {
				case Player.STATE_IDLE:
				case Player.STATE_PAUSED:
					Intent intent = new Intent(PlayerService.RESUME_PLAYER);
					startService(intent);
					break;
				}
				setPlayerLoading();
			}
		});
        
        final Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				switch (Player.getInstance(KaixinPlayerAndroidActivity.this).getState()) {
				case Player.STATE_OPEN:
				case Player.STATE_PLAYING:
					Intent intent = new Intent(PlayerService.PAUSE_PLAYER);
					startService(intent);
					setPlayerPaused();
					break;
				}
			}
		});
        
        
        setPlayerIdle();
        /*
        //String uri = "/data/data/com.kaixindev.kxplayer/app_appdata/legend.mp3";
        String uri = "rtsp://a17.l211053182.c2110.a.lm.akamaistream.net/D/17/2110/v0001/reflector:53182";
        //String uri = "http://www.liming2009.com/108/music/ruguonishiwodechuanshuo.wma";
        Intent intent = new Intent(PlayerService.START_PLAYER);
        intent.putExtra(PlayerService.PROPERTY_URI, uri);
        startService(intent);
        */
    }
    
    @Override
    public void onResume() {
    	super.onResume();
    	IntentFilter filter = new IntentFilter();
    	filter.addAction(UPDATE_PLAYER_CONTROL);
    	registerReceiver(mPCBReceiver, filter);
    }
    
    @Override
    public void onPause() {
    	super.onPause();
    	unregisterReceiver(mPCBReceiver);
    }
    
    public void setPlayerIdle()  {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.GONE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.GONE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.GONE);
    }
    
    public void setPlayerStarted() {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.GONE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.GONE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.VISIBLE);
    }
    
    public void setPlayerPaused() {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.GONE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.VISIBLE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.GONE);   	
    }
    
    public void setPlayerLoading() {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.VISIBLE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.GONE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.VISIBLE);    	
    }
}














