/****************************************************************************
 * This file is part of Bluetooth LE Analyzer.                              *
 * <p/>                                                                     *
 * Copyright (C) 2016  Bertrand Martel                                      *
 * <p/>                                                                     *
 * Foobar is free software: you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 * <p/>                                                                     *
 * Foobar is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 * <p/>                                                                     *
 * You should have received a copy of the GNU General Public License        *
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.          *
 */
package com.github.akinaru.roboticbuttonpusher.service;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;

import com.github.akinaru.roboticbuttonpusher.bluetooth.BluetoothCustomManager;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothEvents;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothObject;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.IPushListener;
import com.github.akinaru.roboticbuttonpusher.bluetooth.rfduino.IRfduinoDevice;
import com.github.akinaru.roboticbuttonpusher.constant.SharedPrefConst;
import com.github.akinaru.roboticbuttonpusher.inter.IPushBtnListener;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherError;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherState;

import java.util.ArrayList;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

/**
 * Service persisting bluetooth connection
 *
 * @author Bertrand Martel
 */
public class BtPusherService extends Service {

    private String TAG = BtPusherService.class.getSimpleName();

    /**
     * load native module entry point
     */
    static {
        System.loadLibrary("buttonpusher");
    }

    public static native byte[] encrypt(String message);

    /**
     * Service binder
     */
    private final IBinder mBinder = new LocalBinder();

    private String mDeviceAdress;

    private ScheduledFuture mTimeoutTask;

    private ButtonPusherState mState = ButtonPusherState.NONE;

    private IPushBtnListener mListener;

    private static final int CONNECTION_TIMEOUT = 5000;

    private static final int SCAN_TIMEOUT = 2500;

    private static final int NOTIFICATION_TIMEOUT = 3000;

    private static final int BLUETOOTH_STATE_TIMEOUT = 2500;

    private Handler mHandler = new Handler(Looper.getMainLooper());

    protected ScheduledExecutorService mExecutor = Executors.newScheduledThreadPool(1);

    public void setListener(IPushBtnListener listener) {
        this.mListener = listener;
    }

    public void setDeviceName(String deviceName) {
        this.mDeviceName = deviceName;
    }

    public void setPassword(String password) {
        this.mPassword = password;
    }

    /*
     * LocalBInder that render public getService() for public access
     */
    public class LocalBinder extends Binder {
        public BtPusherService getService() {
            return BtPusherService.this;
        }
    }

    private BluetoothCustomManager btManager = null;

    private String mDeviceName;

    private String mPassword;

    @Override
    public void onCreate() {

        Log.i(TAG, "service create");

        //initiate bluetooth manager object used to manage all Android Bluetooth API
        btManager = new BluetoothCustomManager(this);

        //initialize bluetooth adapter
        btManager.init(this);

        //register bluetooth event broadcast receiver
        registerReceiver(mBluetoothReceiver, makeGattUpdateIntentFilter());

        SharedPreferences pref = getSharedPreferences(SharedPrefConst.PREFERENCES, Context.MODE_PRIVATE);
        mDeviceName = pref.getString(SharedPrefConst.DEVICE_NAME_FIELD, SharedPrefConst.DEFAULT_DEVICE_NAME);
        mPassword = pref.getString(SharedPrefConst.DEVICE_PASSWORD_FIELD, SharedPrefConst.DEFAULT_PASSWORD);

    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mBluetoothReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    /**
     * broadcast receiver to receive bluetooth events
     */
    private final BroadcastReceiver mBluetoothReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {

            final String action = intent.getAction();

            if (BluetoothEvents.BT_EVENT_SCAN_START.equals(action)) {

                Log.v(TAG, "Scan has started");
            } else if (BluetoothEvents.BT_EVENT_DEVICE_REMOVED.equals(action)) {

                Log.i(TAG, "received : BT_EVENT_DEVICE_REMOVED");
                switch (mState) {
                    case WAIT_DEVICE_START:
                        mState = ButtonPusherState.NONE;
                        startPush.run();
                        break;
                    default:
                        break;
                }

            } else if (BluetoothEvents.BT_EVENT_SCAN_END.equals(action)) {

                Log.v(TAG, "Scan has ended");
                if (mState == ButtonPusherState.DEVICE_FOUND) {
                    mState = ButtonPusherState.CONNECT;

                    changeState(mState);

                    mTimeoutTask = mExecutor.schedule(new Runnable() {
                        @Override
                        public void run() {
                            dispatchError(ButtonPusherError.CONNECTION_TIMEOUT);
                            btManager.disconnectAndRemove(mDeviceAdress);
                            mState = ButtonPusherState.NONE;
                            changeState(mState);

                        }
                    }, CONNECTION_TIMEOUT, TimeUnit.MILLISECONDS);

                    Log.v(TAG, "btManager => " + btManager.getConnectionList().size());
                    btManager.connect(mDeviceAdress);
                }
            } else if (BluetoothEvents.BT_EVENT_DEVICE_RETRY.equals(action)) {
                Log.i(TAG, "RETRYING");
                btManager.connect(mDeviceAdress);
            } else if (BluetoothEvents.BT_EVENT_DEVICE_DISCOVERED.equals(action)) {

                if (mState == ButtonPusherState.SCAN) {

                    Log.v(TAG, "New device has been discovered");

                    final BluetoothObject btDeviceTmp = BluetoothObject.parseArrayList(intent);

                    if (btDeviceTmp != null && btDeviceTmp.getDeviceName().equals(mDeviceName)) {
                        Log.v(TAG, "found device " + mDeviceName);
                        if (mTimeoutTask != null) {
                            mTimeoutTask.cancel(true);
                        }
                        mState = ButtonPusherState.DEVICE_FOUND;
                        changeState(mState);
                        mDeviceAdress = btDeviceTmp.getDeviceAddress();
                        btManager.stopScan();
                    }
                }

            } else if (BluetoothEvents.BT_EVENT_DEVICE_NOTIFICATION.equals(action)) {

                Log.v(TAG, "Device notification : " + mState);
                if (mState == ButtonPusherState.SEND_COMMAND) {

                    if (mTimeoutTask != null) {
                        mTimeoutTask.cancel(true);
                    }

                    btManager.disconnectAndRemove(mDeviceAdress);
                    ArrayList<String> actionsStr = intent.getStringArrayListExtra("");
                    Log.v(TAG, "Receive notification : " + actionsStr.get(0));
                    mState = ButtonPusherState.PROCESS_END;
                    changeState(mState);
                    mState = ButtonPusherState.NONE;
                    changeState(mState);
                }
            } else if (BluetoothEvents.BT_EVENT_DEVICE_DISCONNECTED.equals(action)) {

                Log.v(TAG, "Device disconnected : " + mState);
                if (mState == ButtonPusherState.CONNECT) {
                    mState = ButtonPusherState.NONE;
                    changeState(mState);
                    dispatchError(ButtonPusherError.CONNECTION_TIMEOUT);
                    if (mTimeoutTask != null) {
                        mTimeoutTask.cancel(true);
                    }
                }
            } else if (BluetoothEvents.BT_EVENT_DEVICE_CONNECTED.equals(action)) {

                Log.v(TAG, "device has been connected : " + mState);

                if (mState == ButtonPusherState.CONNECT) {

                    mState = ButtonPusherState.SEND_COMMAND;
                    changeState(mState);
                    if (mTimeoutTask != null) {
                        mTimeoutTask.cancel(true);
                    }

                    if (btManager.getConnectionList().get(mDeviceAdress).getDevice() instanceof IRfduinoDevice) {

                        IRfduinoDevice device = (IRfduinoDevice) btManager.getConnectionList().get(mDeviceAdress).getDevice();

                        mTimeoutTask = mExecutor.schedule(new Runnable() {
                            @Override
                            public void run() {
                                dispatchError(ButtonPusherError.NOTIFICATION_TIMEOUT);
                                btManager.disconnectAndRemove(mDeviceAdress);
                                mState = ButtonPusherState.NONE;
                                changeState(mState);

                            }
                        }, NOTIFICATION_TIMEOUT, TimeUnit.MILLISECONDS);

                        device.sendPush(mPassword, new IPushListener() {
                            @Override
                            public void onPushFailure() {
                            }

                            @Override
                            public void onPushSuccess() {
                            }
                        });
                    }
                }
            } else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {

                final int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                        BluetoothAdapter.ERROR);

                if (state == BluetoothAdapter.STATE_OFF) {

                    Log.e(TAG, "Bluetooth state change to STATE_OFF");

                } else if (state == BluetoothAdapter.STATE_ON) {

                    if (mState == ButtonPusherState.WAIT_BLUETOOTH_STATE) {
                        if (mTimeoutTask != null) {
                            mTimeoutTask.cancel(true);
                        }
                        push.run();
                    }
                }
            }
        }
    };


    private void changeState(final ButtonPusherState state) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mListener != null) {
                    mListener.onChangeState(state);
                }
            }
        });
    }

    private void dispatchError(final ButtonPusherError error) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mListener != null) {
                    mListener.onError(error);
                }
            }
        });
    }

    /**
     * add filter to intent to receive notification from bluetooth service
     *
     * @return intent filter
     */
    private static IntentFilter makeGattUpdateIntentFilter() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothEvents.BT_EVENT_SCAN_START);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_SCAN_END);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_DISCOVERED);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_CONNECTED);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_DISCONNECTED);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_NOTIFICATION);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_REMOVED);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_RETRY);
        intentFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        return intentFilter;
    }

    public Runnable startPush = new Runnable() {
        @Override
        public void run() {

            mTimeoutTask = mExecutor.schedule(new Runnable() {
                @Override
                public void run() {
                    btManager.stopScan();
                    btManager.clearScanningList();
                    mState = ButtonPusherState.NONE;
                    changeState(mState);
                    dispatchError(ButtonPusherError.SCAN_TIMEOUT);

                }
            }, SCAN_TIMEOUT, TimeUnit.MILLISECONDS);

            mState = ButtonPusherState.SCAN;

            btManager.scanLeDevice();
        }
    };

    public Runnable push = new Runnable() {
        @Override
        public void run() {
            Log.v(TAG, "start scan");

            btManager.stopScan();
            btManager.clearScanningList();

            if (btManager.getConnectionList().containsKey(mDeviceAdress)) {

                boolean status = btManager.disconnectAndRemove(mDeviceAdress);

                if (status) {
                    mState = ButtonPusherState.WAIT_DEVICE_START;
                    return;
                }
            }
            startPush.run();
        }
    };

    public void startPushTask() {

        if (mState == ButtonPusherState.NONE) {

            if (!BluetoothAdapter.getDefaultAdapter().isEnabled()) {
                mTimeoutTask = mExecutor.schedule(new Runnable() {
                    @Override
                    public void run() {
                        dispatchError(ButtonPusherError.BLUETOOTH_STATE_TIMEOUT);
                        mState = ButtonPusherState.NONE;
                        changeState(mState);

                    }
                }, BLUETOOTH_STATE_TIMEOUT, TimeUnit.MILLISECONDS);
                mState = ButtonPusherState.WAIT_BLUETOOTH_STATE;
                BluetoothAdapter.getDefaultAdapter().enable();
            } else {
                push.run();
            }
        }
    }

    public void stopPushTask() {

        if (mTimeoutTask != null) {
            mTimeoutTask.cancel(true);
        }
        mListener = null;
        btManager.stopScan();
        btManager.clearScanningList();
        btManager.disconnectAndRemove(mDeviceAdress);
        mState = ButtonPusherState.NONE;
    }

    public void uploadPassword(String pass) {
    }

}