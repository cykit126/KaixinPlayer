package com.kaixindev.kxplayer;

import com.kaixindev.serialize.Attribute;

public class UpdateInfo {
	@Attribute
	public int versionCode;
	
	@Attribute
	public String versionString;
	
	@Attribute
	public String description;
	
	@Attribute
	public String packageUri;
}
