package com.kaixindev.kxplayer;

import java.util.Locale;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class ChannelListAdapter extends BaseAdapter {
	
	private Channel[] mChannels;
	private Context mContext;
	
	private ChannelListAdapter(Channel[] channels, Context context) {
		mChannels = channels;
		mContext = context;
	}
	
	public static ChannelListAdapter create(Channel[] channels, Context context) {
		if (channels == null || context == null) {
			return null;
		} else {
			return new ChannelListAdapter(channels, context);
		}
	}

	@Override
	public int getCount() {
		return mChannels.length;
	}

	@Override
	public Object getItem(int position) {
		return mChannels[position];
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		View view = convertView;
		if (convertView == null) {
			LayoutInflater inflater = (LayoutInflater) mContext
					.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			view = inflater.inflate(R.layout.channel_item, null);
		}
		
		String lang = getLanguage();
		
		TextView titleView = (TextView)view.findViewById(R.id.name);
		Channel channel = (Channel) getItem(position);
		titleView.setText(channel.name != null ? channel.name.get(lang) : null);
		
		return view;
	}

	
	private String getLanguage() {
		return Locale.getDefault().toString();
	}
}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
