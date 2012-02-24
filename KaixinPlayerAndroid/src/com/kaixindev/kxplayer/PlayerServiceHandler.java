package com.kaixindev.kxplayer;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.kaixindev.android.service.IntentHandler;
import com.kaixindev.core.StringUtil;

public class PlayerServiceHandler extends NoProGuardObject {
	
	public static final String LOGTAG = "PlayerServiceHandler";
	
	private PlayerService mService;
	
	public PlayerServiceHandler(PlayerService service) {
		mService = service;
	}
	
	@IntentHandler(action=PlayerService.STOP_PLAYER)
	public void stopPlayer(final Intent intent, final Context context, final Object jobArg) {
		Player.getInstance(mService).stop();
	}
	
	@IntentHandler(action=PlayerService.PAUSE_PLAYER)
	public void pausePlayer(final Intent intent, final Context context, final Object jobArg) {
		Player player = Player.getInstance(mService);
		player.pause();
	}
	
	@IntentHandler(action=PlayerService.RESUME_PLAYER)
	public void resumePlayer(final Intent intent, final Context context, final Object jobArg) {
		Player player = Player.getInstance(mService);
		player.resume();
	}
	
	@IntentHandler(action=PlayerService.START_PLAYER)
	public void startPlayer(final Intent intent, final Context context, final Object jobArg) {
		String uri = intent.getStringExtra(PlayerService.PROPERTY_URI);
		if (StringUtil.isEmpty(uri)) {
			Log.e(LOGTAG, "uri is empty.");
			return;
		}
		
		Player player = Player.getInstance(mService);
		player.play(uri, intent.getStringExtra(PlayerService.PROPERTY_NAME));
	}
}
