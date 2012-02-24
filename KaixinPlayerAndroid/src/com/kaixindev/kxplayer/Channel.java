package com.kaixindev.kxplayer;

import com.kaixindev.core.I18NString;
import com.kaixindev.serialize.Attribute;

public class Channel extends NoProGuardObject {
	@Attribute
	public String areaId;
	
	@Attribute
	public String uri;
	
	@Attribute
	public I18NString name;
}
