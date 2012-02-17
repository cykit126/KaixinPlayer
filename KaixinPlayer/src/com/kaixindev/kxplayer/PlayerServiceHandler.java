package com.kaixindev.kxplayer;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.kaixindev.android.service.IntentHandler;
import com.kaixindev.core.StringUtil;

public class PlayerServiceHandler {
	
	public static final String LOGTAG = "PlayerServiceHandler";
	
	private PlayerService mService;
	
	public PlayerServiceHandler(PlayerService service) {
		mService = service;
	}
	
	@IntentHandler(action=PlayerService.STOP_PLAYER)
	public void stopPlayer(final Intent intent, final Context context, final Object jobArg) {
		Player.getInstance(mService).stop();
		sendUpdateControlNotice();
	}
	
	@IntentHandler(action=PlayerService.PAUSE_PLAYER)
	public void pausePlayer(final Intent intent, final Context context, final Object jobArg) {
		Player player = Player.getInstance(mService);
		player.pause();
		sendUpdateControlNotice();
	}
	
	@IntentHandler(action=PlayerService.RESUME_PLAYER)
	public void resumePlayer(final Intent intent, final Context context, final Object jobArg) {
		Player player = Player.getInstance(mService);
		player.resume();
		sendUpdateControlNotice();		
	}
	
	@IntentHandler(action=PlayerService.START_PLAYER)
	public void startPlayer(final Intent intent, final Context context, final Object jobArg) {
		String uri = intent.getStringExtra(PlayerService.PROPERTY_URI);
		if (StringUtil.isEmpty(uri)) {
			Log.e(LOGTAG, "uri is empty.");
			sendUpdateControlNotice();
			return;
		}
		
		Player player = Player.getInstance(mService);
		player.play(uri);
		
		sendUpdateControlNotice();
	}
	
	private void sendUpdateControlNotice() {
		Intent notice = new Intent(KaixinPlayerAndroidActivity.UPDATE_PLAYER_CONTROL);
		mService.sendBroadcast(notice);
	}
}
