package com.kaixindev.kxplayer;

import java.io.File;
import java.io.InputStream;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.widget.RemoteViews;

import com.flurry.android.FlurryAgent;
import com.kaixindev.android.Application;
import com.kaixindev.android.FileSystem;
import com.kaixindev.android.Log;
import com.kaixindev.core.Hash;
import com.kaixindev.core.IOUtil;
import com.kaixindev.core.ThreadWorker;
import com.kaixindev.io.Copier;
import com.kaixindev.io.FileOutputBuffer;
import com.kaixindev.io.InputBuffer;
import com.kaixindev.io.OutputBuffer;
import com.kaixindev.net.HTTPDownloadBuilder;
import com.kaixindev.serialize.XMLSerializer;

public class CommonService extends Service {
	
	public static final String LOGTAG = "CommonService";
	public static final String ACTION = "com.kaixindev.kxplayer.COMMON_SERVICE";
	
	private CommonServiceBinder mBinder = new CommonServiceBinder();
	private ThreadWorker mThreadWorker = new ThreadWorker();
	
	public CommonService() {
		mThreadWorker.start();
	}
	
	public class CommonServiceBinder extends Binder {
		public CommonService getService() {
			return CommonService.this;
		}
	}
	
	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}

	public void checkUpdate(final Activity activity, final Handler handler) {
		mThreadWorker.pushJob(new Runnable() {
			@Override
			public void run() {				
				URL oUrl = null;
		        try {
		            oUrl = new URL(Config.UPDATE_URI);
		            InputStream input = oUrl.openStream();
		            byte[] content = IOUtil.read(input);
		            XMLSerializer serializer = new XMLSerializer();
		            final UpdateInfo info = (UpdateInfo)serializer.unserialize(content);
			        if (info != null) {
						int versionCode = Application
								.getVersionCode(CommonService.this);
						if (versionCode < info.versionCode) {
							handler.post(new Runnable() {
								@Override
								public void run() {
									AlertDialog dialog = new AlertDialog.Builder(
											activity)
											.setMessage(R.string.comfirm_update)
											.setPositiveButton(
													R.string.update_yes,
													new DialogInterface.OnClickListener() {
														@Override
														public void onClick(
																DialogInterface dialog,
																int which) {
															downloadUpdate(info);
														}
													})
											.setNegativeButton(
													R.string.update_no, null)
											.create();
									dialog.show();
								}
							});
						}
					}

		        }
		        catch (Exception e) {
		        	Log.w(e.getMessage());
		            e.printStackTrace();
		            return;
		        }
		    }
		});
	}
	
	public void downloadUpdate(final UpdateInfo info) {
		Map<String,String> args = new HashMap<String,String>();
		args.put("version", info.versionString);
		args.put("current version", Application.getVersionName(this));
		FlurryAgent.logEvent(Config.FLURRY_EVENT_UPDATE, args, true);
		mThreadWorker.pushJob(new Runnable(){
			@Override
			public void run() {
				String filename = Hash.MD5(info.packageUri.getBytes());
				File logFile = FileSystem.getProperCacheFile(
						CommonService.this, Config.DIR_UPDATE, filename+".log");
				if (!IOUtil.createFile(logFile)) {
					Log.e(LOGTAG, "failed to create log file for update.");
					return;
				}
				File file = FileSystem.getProperCacheFile(
						CommonService.this, Config.DIR_UPDATE, filename + ".apk");
				if (!IOUtil.createFile(file)) {
					Log.e(LOGTAG, "failed to create file for update.");
					return;
				}
				FileOutputBuffer buffer = FileOutputBuffer.newInstance(file, "rw");
				HTTPDownloadBuilder builder = new HTTPDownloadBuilder();
				Copier copier = builder.build(info.packageUri, buffer, logFile);
				copier.addOnProgressListener(new Copier.OnProgressListener() {
					@Override
					public boolean onProgress(InputBuffer input, OutputBuffer output, long current) {
						Resources r = getResources();
						String title = r.getString(R.string.update_notification_title, r.getText(R.string.app_name));
						RemoteViews view = new RemoteViews(getPackageName(), R.layout.update_notification);
						view.setProgressBar(R.id.progress, (int)input.getSize(), (int)current, input.getSize()<=0);
						view.setTextViewText(R.id.title, title);
						Notification not = new Notification(R.drawable.icon, title, System.currentTimeMillis());
						not.contentView = view;
						not.flags = Notification.FLAG_ONGOING_EVENT;
						not.contentIntent = PendingIntent.getActivity(CommonService.this, 0, new Intent(), 0);
						getNotificationManager().notify(Config.UPDATE_NOTIFICATION_ID, not);
						return true;
					}
				});
				copier.addOnEndListener(new Copier.OnEndListener() {
					@Override
					public void onEnd(InputBuffer input, OutputBuffer output, long size) {
						Resources r = getResources();
						String title = r.getString(R.string.app_name);
						String content = r.getString(R.string.update_notification_confirm_installation);
						Notification not = new Notification(R.drawable.icon, title, System.currentTimeMillis());
						Intent intent = Application.buildInstallIntent(CommonService.this, output.getPath());
						not.setLatestEventInfo(
								CommonService.this, 
								title, 
								content, 
								PendingIntent.getActivity(CommonService.this, 0, intent, 0));
						not.flags = Notification.FLAG_AUTO_CANCEL;
						getNotificationManager().notify(Config.UPDATE_NOTIFICATION_ID, not);
						//Application.install(CommonService.this, output.getPath());
					}
				});
				if (copier.copy() < 0) {
					Log.e(LOGTAG, "failed to download update apk.");
				}
			}});
	}
	
	private NotificationManager getNotificationManager() {
		return (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
	}
}
