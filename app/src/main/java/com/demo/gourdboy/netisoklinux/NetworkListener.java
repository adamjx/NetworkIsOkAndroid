package com.demo.gourdboy.netisoklinux;

/**
 * Created by GourdBoy on 2017/8/11.
 */

public interface NetworkListener
{
	void networkStateChanged(boolean isNetOk,String delayTime);
}
