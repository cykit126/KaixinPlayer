package com.kaixindev.kxplayer;

import android.content.Intent;

import com.kaixindev.android.service.AsyncService;

public class PlayerService extends AsyncService {
	
	public static final String START_PLAYER = "com.kaixindev.kxplayer.START_PLAYER";
	public static final String PAUSE_PLAYER = "com.kaixindev.kxplayer.PAUSE_PLAYER";
	public static final String RESUME_PLAYER = "com.kaixindev.kxplayer.RESUME_PLAYER";
	public static final String STOP_PLAYER = "com.kaixindev.kxolayer.STOP_PLAYER";
	public static final String PROPERTY_URI = "uri";
	public static final String PROPERTY_RESTART = "restart";
	
	private static final String __PROPERTY_TIMESTAMP = "__timestamp";
	
	@Override
	public void onCreate() {
		super.onCreate();
		mIntentDispatcher.registerHandlers(new PlayerServiceHandler(this));
	}

	public static class Message {
		public long timestamp;
		public Object data;
		public Message(Object data) {
			this.data = data;
			timestamp = System.currentTimeMillis();
		}
	}
	
	public static long getTimestamp(Intent intent) {
		return intent.getLongExtra(__PROPERTY_TIMESTAMP, 0);
	}
	
	protected Intent filter(Intent intent) {
		intent.putExtra(__PROPERTY_TIMESTAMP, System.currentTimeMillis() + 100);
		return intent;
	}
}
