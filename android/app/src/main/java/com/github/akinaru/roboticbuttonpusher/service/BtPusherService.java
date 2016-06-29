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
import com.github.akinaru.roboticbuttonpusher.bluetooth.rfduino.IRfduinoDevice;
import com.github.akinaru.roboticbuttonpusher.constant.SharedPrefConst;
import com.github.akinaru.roboticbuttonpusher.inter.IPushBtnListener;
import com.github.akinaru.roboticbuttonpusher.model.BtnPusherInputTask;
import com.github.akinaru.roboticbuttonpusher.model.BtnPusherKeysType;
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

    private String DEFAULT_MESSAGE = "undefined";

    /**
     * load native module entry point
     */
    static {
        System.loadLibrary("buttonpusher");
    }

    private String topMessage = DEFAULT_MESSAGE;

    private String bottomMessage = DEFAULT_MESSAGE;

    public static native byte[] encrypt(byte[] message, int length, byte[] key, byte[] iv);

    public static native byte[] decrypt(byte[] cipher, int length, byte[] key, byte[] iv);

    public static native byte[] generatekey(byte[] code);

    public static native byte[] generateiv(byte[] code);

    private BtnPusherInputTask mTaskState = BtnPusherInputTask.PUSH;

    private String oldPwd;

    private boolean mAssociate = false;

    /**
     * Service binder
     */
    private final IBinder mBinder = new LocalBinder();

    private String mDeviceAdress;

    private ScheduledFuture mTimeoutTask;

    private ButtonPusherState mState = ButtonPusherState.NONE;

    private IPushBtnListener mListener;

    private static final int CONNECTION_TIMEOUT = 5000;

    private static final int SCAN_TIMEOUT = 5000;

    private static final int PUSH_TIMEOUT = 10000;

    private static final int REQUEST_TIMEOUT = 10000;

    private static final int USER_CODE_TIMEOUT = 60000;

    private static final int BLUETOOTH_STATE_TIMEOUT = 2500;

    private Handler mHandler = new Handler(Looper.getMainLooper());

    protected ScheduledExecutorService mExecutor = Executors.newScheduledThreadPool(1);

    public void setListener(IPushBtnListener listener) {
        this.mListener = listener;
    }

    public void setPassword(String password) {
        this.mPassword = password;
    }

    public void sendAssociationCode(String code) {

        if (mTimeoutTask != null) {
            mTimeoutTask.cancel(true);
        }

        mTimeoutTask = mExecutor.schedule(new Runnable() {
            @Override
            public void run() {
                dispatchError(ButtonPusherError.NOTIFICATION_TIMEOUT);
                btManager.disconnectAndRemove(mDeviceAdress);
                mState = ButtonPusherState.NONE;
                changeState(mState);

            }
        }, PUSH_TIMEOUT, TimeUnit.MILLISECONDS);

        IRfduinoDevice device = (IRfduinoDevice) btManager.getConnectionList().get(mDeviceAdress).getDevice();

        if (device != null) {
            device.sendAssociationCode(code);
        } else {
            Log.e(TAG, "device not found");
        }
    }

    public void sendAssociationCodeFail() {
        Log.e(TAG, "association fail");
        if (mTimeoutTask != null) {
            mTimeoutTask.cancel(true);
        }
        dispatchError(ButtonPusherError.CONNECTION_TIMEOUT);
        btManager.disconnectAndRemove(mDeviceAdress);
        mState = ButtonPusherState.NONE;
        changeState(mState);
    }

    public void generateNewAesKey() {
        uploadGeneratedKeys();
    }

    public void generateDefaultAesKey() {
        uploadDefaultKeys();
    }

    public void disassociate() {
        mTaskState = BtnPusherInputTask.DISASSOCIATE;
        chainTasks();
    }

    public String getTopMessage() {
        return topMessage;
    }

    public String getBottomMessage() {
        return bottomMessage;
    }

    public void setMessage(String topMessage, String bottomMessage) {

        this.topMessage = topMessage;
        this.bottomMessage = bottomMessage;
        Log.i(TAG, "setting top message");
        mTaskState = BtnPusherInputTask.MESSAGE;
        chainTasks();
    }

    public void setAssociate(boolean state) {
        mAssociate = state;
    }

    public boolean isAssociate() {
        return mAssociate;
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

    private String mPassword;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "service create");

        //initiate bluetooth manager object used to manage all Android Bluetooth API
        btManager = new BluetoothCustomManager(this);

        //initialize bluetooth adapter
        btManager.init(this);

        //register bluetooth event broadcast receiver
        registerReceiver(mBluetoothReceiver, makeGattUpdateIntentFilter());

        SharedPreferences pref = getSharedPreferences(SharedPrefConst.PREFERENCES, Context.MODE_PRIVATE);
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

                    if (btDeviceTmp != null && btDeviceTmp.getDeviceName().equals("RFduino")) {
                        Log.v(TAG, "found device ");
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

                        switch (mTaskState) {
                            case PUSH:
                                setTimeoutTask(ButtonPusherError.NOTIFICATION_TIMEOUT, PUSH_TIMEOUT);
                                device.sendPush(mPassword);
                                break;
                            case PASSWORD:
                                setTimeoutTask(ButtonPusherError.SET_PASSWORD_TIMEOUT, REQUEST_TIMEOUT);
                                device.setPassword(oldPwd);
                                break;
                            case KEYS_DEFAULT:
                                setTimeoutTask(ButtonPusherError.SET_KEYS_TIMEOUT, REQUEST_TIMEOUT);
                                device.setKeys(mPassword, BtnPusherKeysType.DEFAULT);
                                break;
                            case KEYS_GENERATED:
                                setTimeoutTask(ButtonPusherError.SET_KEYS_TIMEOUT, REQUEST_TIMEOUT);
                                device.setKeys(mPassword, BtnPusherKeysType.GENERATED);
                                break;
                            case MESSAGE:
                                setTimeoutTask(ButtonPusherError.SET_MESSAGE_TIMEOUT, REQUEST_TIMEOUT);
                                device.setMessage(topMessage, bottomMessage);
                                break;
                            case DISASSOCIATE:
                                setTimeoutTask(ButtonPusherError.DISASSOCIATE_TIMEOUT, REQUEST_TIMEOUT);
                                device.disassociate();
                                break;
                        }
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
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_USER_ACTION_REQUIRED)) {
                Log.v(TAG, "removing timout");
                if (mTimeoutTask != null) {
                    mTimeoutTask.cancel(true);
                }
                Log.v(TAG, "setting large timeout to let user type code");

                mTimeoutTask = mExecutor.schedule(new Runnable() {
                    @Override
                    public void run() {
                        dispatchError(ButtonPusherError.NOTIFICATION_TIMEOUT);
                        btManager.disconnectAndRemove(mDeviceAdress);
                        mState = ButtonPusherState.NONE;
                        changeState(mState);

                    }
                }, USER_CODE_TIMEOUT, TimeUnit.MILLISECONDS);

            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_SUCCESS)) {
                sendSuccess();
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_FAILURE)) {
                sendFailure(ButtonPusherError.ASSOCIATION_FAILURE);
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_PUSH_SUCCESS)) {
                sendSuccess();
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_SUCCESS)) {
                sendSuccess();
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_SUCCESS)) {
                sendSuccess();
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_SUCCESS)) {
                sendSuccess();
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_PUSH_FAILURE)) {
                sendFailure(ButtonPusherError.PUSH_FAILURE);
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_FAILURE)) {
                sendFailure(ButtonPusherError.SET_KEYS_FAILURE);
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_FAILURE)) {
                sendFailure(ButtonPusherError.SET_PASSWORD_FAILURE);
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_FAILURE)) {
                sendFailure(ButtonPusherError.DISASSOCIATE_FAILURE);
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_SET_MESSAGE_SUCCESS)) {
                sendSuccess();
            } else if (action.equals(BluetoothEvents.BT_EVENT_DEVICE_MESSAGE_FAILURE)) {
                sendFailure(ButtonPusherError.SET_MESSAGE_FAILURE);
            }
        }
    };

    private void setTimeoutTask(final ButtonPusherError error, int timeout) {

        mTimeoutTask = mExecutor.schedule(new Runnable() {
            @Override
            public void run() {
                dispatchError(error);
                btManager.disconnectAndRemove(mDeviceAdress);
                mState = ButtonPusherState.NONE;
                changeState(mState);

            }
        }, timeout, TimeUnit.MILLISECONDS);
    }

    private void sendSuccess() {
        if (mTimeoutTask != null) {
            mTimeoutTask.cancel(true);
        }

        btManager.disconnectAndRemove(mDeviceAdress);
        Log.i(TAG, "change state PROCESS_END");
        mState = ButtonPusherState.PROCESS_END;
        changeState(mState);
        mState = ButtonPusherState.NONE;
        changeState(mState);
    }

    private void sendFailure(ButtonPusherError error) {

        if (mTimeoutTask != null) {
            mTimeoutTask.cancel(true);
        }
        dispatchError(error);
        btManager.disconnectAndRemove(mDeviceAdress);
        mState = ButtonPusherState.NONE;
        changeState(mState);
    }

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
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_USER_ACTION_REQUIRED);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_PUSH_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_PUSH_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_MESSAGE_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_MESSAGE_FAILURE);
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
        mTaskState = BtnPusherInputTask.PUSH;
        chainTasks();
    }

    private void chainTasks() {

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
        //mListener = null;
        btManager.stopScan();
        btManager.clearScanningList();
        btManager.disconnectAndRemove(mDeviceAdress);
        mState = ButtonPusherState.NONE;
    }

    public void uploadPassword(String oldPass) {
        this.oldPwd = oldPass;
        Log.i(TAG, "old pass : " + oldPass);
        mTaskState = BtnPusherInputTask.PASSWORD;
        chainTasks();
    }

    public void uploadDefaultKeys() {
        mTaskState = BtnPusherInputTask.KEYS_DEFAULT;
        chainTasks();
    }

    public void uploadGeneratedKeys() {
        mTaskState = BtnPusherInputTask.KEYS_GENERATED;
        chainTasks();
    }

}