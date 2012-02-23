package com.kaixindev.kxplayer;

import android.content.Context;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import com.kaixindev.core.StringUtil;

public class Player implements Agent.OnStartListener, Agent.OnReceiveListener, Agent.OnFinishListener {
	public final String LOGTAG = "Player";
	
	public static final int STATE_IDLE = 1;
	public static final int STATE_OPEN = 2;
	public static final int STATE_PLAYING = 3;
	public static final int STATE_PAUSED = 4;
	public static final int STATE_ERROR = 5;
	
	private int mState = STATE_IDLE;
	private final static Object mLock = new Object();
	
	private AudioTrack mAudioTrack;
	private Agent mAgent;
	private Context mContext;
	private String mProgName;
	
	private Player(Context context) {
		mAgent = Agent.create(this, this, this);
		mContext = context;
	}
	
	public void finalize() {
		stop();
	}
	
	synchronized public int getState() {
		return mState;
	}
	
	synchronized public void setState(int state) {
		mState = state;
		notify(state);
	}
	
	synchronized public boolean play(String uri, String name) {
		if (StringUtil.isEmpty(uri)) {
			Log.e(LOGTAG, "uri is empty.");
			return false;
		}
		abort();
		mProgName = name;
		if (mAgent.open(uri) == 0) {
			setState(STATE_OPEN);
			return true;
		} else {
			setState(STATE_ERROR);
			return false;
		}
	}
	
	synchronized public void resume() {
		if (getState() == STATE_PAUSED) {
			mAgent.resume();
			mAudioTrack.play();
			setState(STATE_PLAYING);
		}
	}
	
	synchronized public void stop() {
		mAudioTrack.release();
		mAudioTrack = null;
		mAgent.release();
		mAgent = null;
	}
	
	synchronized public void pause() {
		int state = getState();
		if (state == STATE_OPEN || state == STATE_PLAYING) {
			mAgent.pause();
			mAudioTrack.pause();
			setState(Player.STATE_PAUSED);
		}
	}
	
	synchronized public void abort() {
		int state = getState();
		if (state == STATE_OPEN || state == STATE_PLAYING) {
			mAudioTrack.release();
			mAudioTrack = null;
			mAgent.abort();
			setState(Player.STATE_IDLE);
		}
	}
	
	@Override
	synchronized public void onReceive(byte[] data) {
		if (mAudioTrack != null) {
			mAudioTrack.write(data, 0, data.length);
		}
	}

	@Override
	synchronized public int onStart(AVContext context) {
        int channels = context.mAudioChannels == 2 ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO;
        int bufferSize = AudioTrack.getMinBufferSize(
        		context.mAudioSampleRate, 
        		channels, 
        		AudioFormat.ENCODING_PCM_16BIT);
        mAudioTrack = new AudioTrack(
				AudioManager.STREAM_MUSIC,
				context.mAudioSampleRate,
				channels,
				AudioFormat.ENCODING_PCM_16BIT,
				bufferSize,
				AudioTrack.MODE_STREAM);
        mAudioTrack.play();
        setState(STATE_PLAYING);      
        
        return 0;
	}

	///////////////////////////////////////////////////////////////////////////
	
	private static Player mInstance;
	
	public static Player getInstance(Context context) {
		synchronized (mLock) {
			if (mInstance == null) {
				mInstance = new Player(context);
			}
			return mInstance;
		}
	}

	@Override
	public void onFinish(int status) {
		setState(status == Agent.STATUS_ERROR ? Player.STATE_ERROR : Player.STATE_IDLE);
	}
	
	public void notify(int status) {
		Intent intent = new Intent(PlayerService.PLAYER_NOTICE);
		intent.putExtra(PlayerService.PROPERTY_STATE, status);
		intent.putExtra(PlayerService.PROPERTY_NAME, mProgName);
		mContext.sendBroadcast(intent);
	}
}
