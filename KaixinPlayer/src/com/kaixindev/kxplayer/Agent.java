package com.kaixindev.kxplayer;

public class Agent {
	public static final int STATE_IDLE 				= 0;
	public static final int STATE_OPEN 				= 1;
	public static final int STATE_STARTED 			= 2;
	public static final int STATE_RECEIVING_DATA 	= 3;
	public static final int STATE_PAUSED 			= 4;
	public static final int STATE_ABORTED 			= 5;
	
	private int mPtrNativeDev = 0;
	
	public interface OnReceiveListener {
		public void onReceive(byte[] data);
	}
	
	public interface OnFinishListener {
		public void onFinish();
	}
	
	public void finalize() {
		release();
	}
	
	native public static Agent create();
	native public int open(String uri, AVContext context);
	native public int start(OnReceiveListener recListener, OnFinishListener finListener);
	native public int pause();
	native public int resume();
	native public int abort();
	native public int getState();
	native public int release();
}
