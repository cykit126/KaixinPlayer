package com.kaixindev.kxplayer;

import android.content.Context;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import com.kaixindev.android.service.IntentHandler;
import com.kaixindev.core.StringUtil;

public class PlayerServiceHandler {
	
	public static final String LOGTAG = "PlayerServiceHandler";
	
	@IntentHandler(action=PlayerService.START_PLAYER)
	public void startPlayer(final Intent intent, final Context context, final Object jobArg) {
		String uri = intent.getStringExtra(PlayerService.PROPERTY_URI);
		if (StringUtil.isEmpty(uri)) {
			Log.e(LOGTAG, "uri is empty.");
			return;
		}
		
	    Agent agent = Agent.create();
        AVContext avctx = new AVContext();
        if (agent.open(uri, avctx) != 0) {
        	Log.e(LOGTAG, "failed to open mp3 file.");
        	return;
        }
        
        int channels = avctx.mAudioChannels == 2 ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO;
        int bufferSize = AudioTrack.getMinBufferSize(
        		avctx.mAudioSampleRate, 
        		channels, 
        		AudioFormat.ENCODING_PCM_16BIT);
        final AudioTrack audioTrack = new AudioTrack(
				AudioManager.STREAM_MUSIC,
				avctx.mAudioSampleRate,
				channels,
				AudioFormat.ENCODING_PCM_16BIT,
				bufferSize,
				AudioTrack.MODE_STREAM);
        audioTrack.play();
        
        Agent.OnReceiveListener onReceiveListener = new Agent.OnReceiveListener() {
			@Override
			public void onReceive(byte[] data) {
				audioTrack.write(data, 0, data.length);
			}
		};
        
        if (agent.start(onReceiveListener, null) != 0) {
        	Log.e(LOGTAG, "failed to start player.");
        }
	}
}
