package com.github.akinaru.roboticbuttonpusher;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.RemoteViews;

import com.github.akinaru.roboticbuttonpusher.constant.Common;

public class ExampleAppWidgetProvider extends AppWidgetProvider {

    private PushSingleton mSingleton;

    public ExampleAppWidgetProvider() {
        mSingleton = PushSingleton.getInstance();
    }

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
        Log.d("TAG", "onUpdate");

        for (int currentWidgetId : appWidgetIds) {
            RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.appwidget_provider);

            views.setOnClickPendingIntent(R.id.start_btn, generatePendingIntent(context, Common.ACTION_START));

            appWidgetManager.updateAppWidget(currentWidgetId, views);
        }
    }

    private PendingIntent generatePendingIntent(Context context, String action) {
        Intent intent = new Intent(context, getClass());
        intent.setAction(action);
        return PendingIntent.getBroadcast(context, 0, intent, 0);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        super.onReceive(context, intent);

        if (intent.getAction().equals(Common.ACTION_START)) {
            Log.i("TAG","onreceive");
            mSingleton.pushOneShot(context.getApplicationContext());
        }
    }
}