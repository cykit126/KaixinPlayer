package com.kaixindev.kxplayer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class PlayerControlBroadcastrReceiver extends BroadcastReceiver {
	
	private KaixinPlayerAndroidActivity mActivity;
	
	public PlayerControlBroadcastrReceiver(KaixinPlayerAndroidActivity act) {
		mActivity = act;
	}
	
	@Override
	public void onReceive(Context arg0, Intent arg1) {
		switch (Player.getInstance(mActivity).getState()) {
		case Player.STATE_PLAYING:
			mActivity.setPlayerStarted();
			break;
		case Player.STATE_IDLE:
			mActivity.setPlayerIdle();
			break;
		case Player.STATE_PAUSED:
			mActivity.setPlayerPaused();
			break;
		case Player.STATE_OPEN:
			mActivity.setPlayerLoading();
			break;
		default:
			break;
		}
	}

}
