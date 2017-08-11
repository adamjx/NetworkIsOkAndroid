package com.demo.gourdboy.netisoklinux;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity
{
	private NativeMethod nativeMethod;
	static {
		System.loadLibrary("IsNetOk");
	}

	private NativeMethod nm;
	private TextView tv_txt;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		nm = NativeMethod.getInstance();
		nm.netOkInit();
		nm.setIP("192.168.2.100");
		nm.netOkThreadStart();
		nm.setNetworkListener(new NetworkListener()
		{
			@Override
			public void networkStateChanged(final boolean isNetOk,final String delayTime)
			{
				runOnUiThread(new Runnable()
				{
					@Override
					public void run()
					{
						tv_txt.append((isNetOk?"connected":"disconnected")+", delay="+delayTime+"ms\n");
					}
				});
			}
		});
		setContentView(R.layout.activity_main);
		Button bt_ping = (Button) findViewById(R.id.bt_ping);
		tv_txt = (TextView) findViewById(R.id.tv_txt);
		bt_ping.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				nm.setIP("192.168.1.83");
			}
		});
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		nm.netOkThreadStop();
	}
}
