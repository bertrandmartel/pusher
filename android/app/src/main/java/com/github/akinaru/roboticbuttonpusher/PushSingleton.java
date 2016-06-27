/**********************************************************************************
 * This file is part of Button Pusher.                                             *
 * <p/>                                                                            *
 * Copyright (C) 2016  Bertrand Martel                                             *
 * <p/>                                                                            *
 * Button Pusher is free software: you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by            *
 * the Free Software Foundation, either version 3 of the License, or               *
 * (at your option) any later version.                                             *
 * <p/>                                                                            *
 * Button Pusher is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   *
 * GNU General Public License for more details.                                    *
 * <p/>                                                                            *
 * You should have received a copy of the GNU General Public License               *
 * along with Button Pusher. If not, see <http://www.gnu.org/licenses/>.           *
 */
package com.github.akinaru.roboticbuttonpusher;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

import com.github.akinaru.roboticbuttonpusher.inter.IPushBtnListener;
import com.github.akinaru.roboticbuttonpusher.inter.ISingletonListener;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherError;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherState;
import com.github.akinaru.roboticbuttonpusher.service.BtPusherService;

/**
 * Singleton used to persist binding to Bluetooth Push button Service.
 *
 * @author Bertrand Martel
 */
public class PushSingleton {

    private static PushSingleton mInstance;

    private static final String TAG = PushSingleton.class.getSimpleName();

    private ISingletonListener mListener;

    /**
     * bluetooth analyzer service
     */
    private BtPusherService mService = null;

    /**
     * define is service is bound or not
     */
    private boolean mBound = false;

    private boolean mOneSHot = false;

    public PushSingleton() {

    }

    public void bindService(Context context) {
        //bind to service
        Intent intent = new Intent(context, BtPusherService.class);
        mBound = context.bindService(intent, mServiceConnection, Activity.BIND_AUTO_CREATE);
    }

    public void unbindService(Context context) {
        try {
            if (mBound) {
                context.unbindService(mServiceConnection);
                mBound = false;
            }
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
    }

    public void pushOneShot(final Context context) {

        if (!mOneSHot) {
            mOneSHot = true;
            mListener = new ISingletonListener() {
                @Override
                public void onBind() {
                    mService.setListener(new IPushBtnListener() {
                        @Override
                        public void onChangeState(ButtonPusherState newState) {

                            switch (newState) {
                                case PROCESS_END:
                                    mOneSHot = false;
                                    Toast.makeText(context, "push success", Toast.LENGTH_SHORT).show();
                                    mService.setListener(null);
                                    unbindService(context);
                                    break;
                                default:
                                    break;
                            }
                        }

                        @Override
                        public void onError(ButtonPusherError error) {
                            mOneSHot = false;
                            Toast.makeText(context, "push failed", Toast.LENGTH_SHORT).show();
                            mService.setListener(null);
                            unbindService(context);
                        }
                    });
                }
            };
            bindService(context);
        } else {
            Log.i(TAG, "already pushing...");
        }
    }

    /**
     * Manage Bluetooth Service
     */
    private ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {

            Log.v(TAG, "connected to service");
            mService = ((BtPusherService.LocalBinder) service).getService();

            if (mListener != null) {
                mListener.onBind();
            }
            if (mOneSHot) {
                mOneSHot = false;
                startPushTask();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        }
    };

    public void setSingletonListener(ISingletonListener listener) {
        mListener = listener;
    }

    public void setServiceListener(IPushBtnListener listener) {
        mService.setListener(listener);
    }

    public static PushSingleton getInstance() {
        if (mInstance == null) {
            Log.i(TAG, "create instance Singleton");
            mInstance = new PushSingleton();
        }
        return mInstance;
    }

    public void stopPushTask() {
        if (mService != null) {
            mService.stopPushTask();
        }
    }

    public void startPushTask() {
        if (mService != null) {
            mService.startPushTask();
        }
    }

    public void setPassword(String password) {
        mService.setPassword(password);
    }

    public void uploadPassword(String oldPass) {
        mService.uploadPassword(oldPass);
    }

    public void sendAssociationCode(String code) {
        mService.sendAssociationCode(code);
    }

    public void sendAssociationCodeFail() {
        mService.sendAssociationCodeFail();
    }

    public void generateNewAesKey() {
        mService.generateNewAesKey();
    }

    public void disassociate() {
        mService.disassociate();
    }

    public String getTopMessage() {
        return mService.getTopMessage();
    }

    public String getBotttomMessage() {
        return mService.getBottomMessage();
    }

    public void setMessage(String topMessage, String bottomMessage) {
        mService.setMessage(topMessage, bottomMessage);
    }
}
