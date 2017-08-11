package com.demo.gourdboy.netisoklinux;

import android.util.Log;

/**
 * Created by GourdBoy on 2017/7/13.
 */

public class NativeMethod
{
	private NetworkListener networkListener;
	private NativeMethod()
	{

	}
	private static class NativeMethodHolder
	{
		public static final  NativeMethod INSTANCE = new NativeMethod();
	}
	public void setNetworkListener(NetworkListener networkListener)
	{
		this.networkListener = networkListener;
	}
	public static NativeMethod getInstance()
	{
		return NativeMethodHolder.INSTANCE;
	}
	public native void netOkInit();
	public native void setIP(String ip);
	public native void netOkThreadStart();
	public native void netOkThreadStop();

	private void isNetOkCallBack(boolean isOk,String delayTime)
	{
		if(networkListener!=null)
		{
			networkListener.networkStateChanged(isOk,delayTime);
		}
		Log.i("xxxxxxxxx","is network ok: "+isOk+" ,time="+delayTime+"ms");
	}
}
