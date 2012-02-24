package com.kaixindev.kxplayer;

public class Device extends NoProGuardObject {
	public interface OnNoticeListener {
		public void onNotice(int event);
	}
	
	public class Option {
		public AVContext context;
		public OnNoticeListener onNoticeListenr;
	}
	
	native public static Device open(Option option);
	native public void pause(boolean isPause);
	native public void close();
	native public int write(byte[] data, int size);
}
