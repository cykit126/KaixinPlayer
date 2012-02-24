package com.kaixindev.kxplayer;

import android.app.TabActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TabHost;
import android.widget.TextView;

import com.flurry.android.FlurryAgent;
import com.kaixindev.kxplayer.CommonService.CommonServiceBinder;

public class PlayerActivity extends TabActivity {
	
	// /data/data/com.kaixindev.kxplayer/app_appdata/legend.mp3
	
	public static final String LOGTAG = "KaixinPlayerAndroidActivity";
	
	static {
		System.loadLibrary("kxplayer");
	}
	
	private Handler mHandler = new Handler();
	private PlayerControlBroadcastrReceiver mPCBReceiver;
	private CommonService mCommonService;
	private boolean mBound = false;
	private ServiceConnection mConn = new ServiceConnection() {

		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mCommonService = ((CommonServiceBinder)service).getService();
			mBound = true;
			mCommonService.checkUpdate(PlayerActivity.this, mHandler);
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mCommonService = null;
		}
		
	};
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        com.kaixindev.android.Log.ENABLED = true;
        
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
				switch (Player.getInstance(PlayerActivity.this).getState()) {
				case Player.STATE_IDLE:
				case Player.STATE_PAUSED:
				case Player.STATE_ERROR:
					Intent intent = new Intent(PlayerService.RESUME_PLAYER);
					startService(intent);
					setPlayerLoading();
					break;
				}
			}
		});
        
        final Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				switch (Player.getInstance(PlayerActivity.this).getState()) {
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
        
        intent = new Intent(CommonService.ACTION);
    	if (!bindService(intent, mConn, Context.BIND_AUTO_CREATE)) {
    		Log.e(LOGTAG, "unable to bind CommonService.");
    	}
    }
    
    @Override
    public void onResume() {
    	super.onResume();
    	IntentFilter filter = new IntentFilter();
    	filter.addAction(PlayerService.PLAYER_NOTICE);
    	registerReceiver(mPCBReceiver, filter);
    }
    
    @Override
    public void onPause() {
    	super.onPause();
    	unregisterReceiver(mPCBReceiver);
    }
    
    @Override
    public void onDestroy() {
    	super.onDestroy();
    	if (mBound) {
    		unbindService(mConn);
    		mCommonService = null;
    		mBound = false;
    	}
    }
    
    @Override
    public void onStart() {
    	super.onStart();
    	FlurryAgent.onStartSession(this, Config.FLURRY_API_KEY);
    }
    
    @Override
    public void onStop() {
    	super.onStop();
    	FlurryAgent.onEndSession(this);
    }
    
    public void setPlayerIdle()  {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.GONE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.GONE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.GONE);
        TextView displayView = (TextView)findViewById(R.id.player_control_display);
        displayView.setVisibility(View.GONE);
    }
    
    public void setPlayerError() {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.GONE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.GONE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.GONE);
        TextView displayView = (TextView)findViewById(R.id.player_control_display);
        displayView.setVisibility(View.VISIBLE);    	
    }
    
    public void setPlayerStarted() {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.GONE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.GONE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.VISIBLE);
        TextView displayView = (TextView)findViewById(R.id.player_control_display);
        displayView.setVisibility(View.VISIBLE);
    }
    
    public void setPlayerPaused() {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.GONE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.VISIBLE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.GONE);
        TextView displayView = (TextView)findViewById(R.id.player_control_display);
        displayView.setVisibility(View.VISIBLE);
    }
    
    public void setPlayerLoading() {
    	ProgressBar pbLoading = (ProgressBar)findViewById(R.id.loading);
        pbLoading.setVisibility(View.VISIBLE);
        Button btnResume = (Button)findViewById(R.id.player_control_start);
        btnResume.setVisibility(View.GONE);
        Button btnPause = (Button)findViewById(R.id.player_control_pause);
        btnPause.setVisibility(View.VISIBLE);
        TextView displayView = (TextView)findViewById(R.id.player_control_display);
        displayView.setVisibility(View.VISIBLE);
    }
    
    public void updatePlayerDisplay(String content) {
    	TextView displayView = (TextView)findViewById(R.id.player_control_display);
    	displayView.setText(content);
    }
}














