package com.kaixindev.kxplayer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;

public class PlayerControlBroadcastrReceiver extends BroadcastReceiver {
	
	private PlayerActivity mActivity;
	
	public PlayerControlBroadcastrReceiver(PlayerActivity act) {
		mActivity = act;
	}
	
	@Override
	public void onReceive(Context context, Intent intent) {
		Resources r = mActivity.getResources();
		switch (Player.getInstance(mActivity).getState()) {
		case Player.STATE_PLAYING:{
			mActivity.setPlayerStarted();
			String content = r.getString(
					R.string.player_control_playing, 
					intent.getStringExtra(PlayerService.PROPERTY_NAME));
			mActivity.updatePlayerDisplay(content);
			break;
		}
		case Player.STATE_IDLE:
			mActivity.setPlayerIdle();
			mActivity.updatePlayerDisplay(null);
			break;
		case Player.STATE_ERROR: {
			mActivity.setPlayerError();
			String content = r.getString(
					R.string.player_control_error, 
					intent.getStringExtra(PlayerService.PROPERTY_NAME));
			mActivity.updatePlayerDisplay(content);			
			break;
		}
		case Player.STATE_PAUSED:
			mActivity.setPlayerPaused();
			break;
		case Player.STATE_OPEN: {
			mActivity.setPlayerLoading();
			String content = r.getString(
					R.string.player_control_loading, 
					intent.getStringExtra(PlayerService.PROPERTY_NAME));
			mActivity.updatePlayerDisplay(content);			
			break;
		}
		default:
			break;
		}
	}

}
