package com.kaixindev.kxplayer;

import com.kaixindev.android.service.AsyncService;

public class PlayerService extends AsyncService {

	public static final String START_PLAYER = "com.kaixindev.kxplayer.START_PLAYER";
	public static final String PROPERTY_URI = "uri";
	
	@Override
	public void onCreate() {
		super.onCreate();
		mIntentDispatcher.registerHandlers(new PlayerServiceHandler());
	}
}
