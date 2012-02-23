package com.kaixindev.kxplayer;

import java.io.InputStream;
import java.util.Locale;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListAdapter;
import android.widget.ListView;

import com.kaixindev.core.IOUtil;
import com.kaixindev.serialize.XMLSerializer;

public class RecommActivity extends ListTabActivity implements AdapterView.OnItemClickListener  {
	
	private Channel[] mChannels;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.tab_list_activity);
		
		InputStream input = IOUtil.open(this, "asset://recommands.xml");
		byte[] content = IOUtil.read(input);
		
		XMLSerializer serializer = new XMLSerializer();
		mChannels = (Channel[])serializer.unserialize(content);
		
		ListAdapter adapter = ChannelListAdapter.create(mChannels, this);
		ListView listView = (ListView) findViewById(R.id.listview);
		listView.setOnItemClickListener(this);
		listView.setAdapter(adapter);
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		ListView listView = (ListView) findViewById(R.id.listview);
		ChannelListAdapter adapter = (ChannelListAdapter)listView.getAdapter();
		Channel channel = (Channel)adapter.getItem(position);
		
		if (listView.isItemChecked(position)) {
			return;
		}

		Intent intent = new Intent(PlayerService.START_PLAYER);
		intent.putExtra(PlayerService.PROPERTY_URI, channel.uri);
		intent.putExtra(PlayerService.PROPERTY_NAME, channel.name.get(Locale.getDefault().toString()));
		intent.putExtra(PlayerService.PROPERTY_RESTART, true);
		startService(intent);
		
		listView.clearChoices();
		listView.setItemChecked(position, true);
	}
}
