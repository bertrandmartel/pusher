/**********************************************************************************
 * This file is part of Pusher.                                                    *
 * <p/>                                                                            *
 * Copyright (C) 2016  Bertrand Martel                                             *
 * <p/>                                                                            *
 * Pusher is free software: you can redistribute it and/or modify                  *
 * it under the terms of the GNU General Public License as published by            *
 * the Free Software Foundation, either version 3 of the License, or               *
 * (at your option) any later version.                                             *
 * <p/>                                                                            *
 * Pusher is distributed in the hope that it will be useful,                       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   *
 * GNU General Public License for more details.                                    *
 * <p/>                                                                            *
 * You should have received a copy of the GNU General Public License               *
 * along with Pusher. If not, see <http://www.gnu.org/licenses/>.                  *
 */
package com.github.akinaru.roboticbuttonpusher;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.RemoteViews;

import com.github.akinaru.roboticbuttonpusher.constant.Common;

/**
 * Widget used to push button even without UI
 *
 * @author Bertrand Martel
 */
public class ButtonWidgetProvider extends AppWidgetProvider {

    private PushSingleton mSingleton;

    public ButtonWidgetProvider() {
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
            Log.v("TAG", "onreceive");
            mSingleton.setAssociate(false);
            mSingleton.pushOneShot(context.getApplicationContext());
        }
    }
}