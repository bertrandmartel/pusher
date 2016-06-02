/*
 * This file is part of Bluetooth LE Analyzer.
 * <p/>
 * Copyright (C) 2016  Bertrand Martel
 * <p/>
 * Foobar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * <p/>
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
package com.github.akinaru.roboticbuttonpusher.activity;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.support.design.widget.FloatingActionButton;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Toast;

import com.github.akinaru.roboticbuttonpusher.R;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothEvents;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothObject;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.IPushListener;
import com.github.akinaru.roboticbuttonpusher.bluetooth.rfduino.IRfduinoDevice;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherState;
import com.github.akinaru.roboticbuttonpusher.service.BtPusherService;

import java.util.ArrayList;


/**
 * Analyzer activity
 *
 * @author Bertrand Martel
 */
public class PushActivity extends BaseActivity {

    private String TAG = this.getClass().getName();

    private static final String DEFAULT_DEVICE_NAME = "RFduino";

    private static final String DEFAULT_PASSWORD = "admin";

    private static final boolean DEFAULT_DEBUG_MODE = false;

    private String mDeviceName;

    private String mPassword;

    private boolean mDebugMode;

    private final static String PREFERENCES = "storage";

    private FloatingActionButton button;

    private String mDeviceAdress;

    private ButtonPusherState state = ButtonPusherState.NONE;

    /**
     * shared preference object.
     */
    private SharedPreferences sharedPref;

    protected void onCreate(Bundle savedInstanceState) {

        setLayout(R.layout.activity_button_push);
        super.onCreate(savedInstanceState);

        //shared preference
        sharedPref = getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);

        mDeviceName = sharedPref.getString("deviceName", DEFAULT_DEVICE_NAME);
        mPassword = sharedPref.getString("password", DEFAULT_PASSWORD);
        mDebugMode = sharedPref.getBoolean("debugMode", DEFAULT_DEBUG_MODE);

        button = (FloatingActionButton) findViewById(R.id.fab);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                state = ButtonPusherState.SCAN;
                clearReplaceDebugTv("Scanning for device ...");
                triggerNewScan();
            }
        });

        //register bluetooth event broadcast receiver
        registerReceiver(mBluetoothReceiver, makeGattUpdateIntentFilter());

        //bind to service
        if (mBluetoothAdapter.isEnabled()) {
            Intent intent = new Intent(this, BtPusherService.class);
            mBound = bindService(intent, mServiceConnection, BIND_AUTO_CREATE);
        }
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        boolean ret = super.onPrepareOptionsMenu(menu);
        nvDrawer.getMenu().findItem(R.id.devicename_item).setTitle(getString(R.string.menu_device_name) + " " + mDeviceName);

        String debugModeStr = mDebugMode ? "enabled" : "disabled";

        nvDrawer.getMenu().findItem(R.id.debug_mode_item).setTitle(getString(R.string.debug_mode_title) + " " + debugModeStr);
        if (mDebugMode) {
            toolbar.getMenu().findItem(R.id.clear_btn).setVisible(true);
            debugTv.setVisibility(View.VISIBLE);
        }
        return ret;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.v(TAG, "onDestroy");
        try {
            if (mBound) {
                unbindService(mServiceConnection);
                mBound = false;
            }
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mService != null) {
            if (mService.isScanning()) {
                mService.stopScan();
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        if (requestCode == REQUEST_ENABLE_BT) {

            if (mBluetoothAdapter.isEnabled()) {
                Intent intent = new Intent(this, BtPusherService.class);
                // bind the service to current activity and create it if it didnt exist before
                startService(intent);
                mBound = bindService(intent, mServiceConnection, BIND_AUTO_CREATE);

            } else {
                Toast.makeText(this, getResources().getString(R.string.toast_bluetooth_disabled), Toast.LENGTH_SHORT).show();
            }
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
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        }
    };

    @Override
    public String getPassword() {
        return mPassword;
    }

    @Override
    public String getDeviceName() {
        return mDeviceName;
    }

    @Override
    public boolean getDebugMode() {
        return mDebugMode;
    }

    @Override
    public Context getContext() {
        return this;
    }

    @Override
    public void setDeviceName(String deviceName) {

        if (deviceName != null && !deviceName.equals("")) {
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.putString("deviceName", deviceName);
            editor.commit();
            mDeviceName = deviceName;
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    nvDrawer.getMenu().findItem(R.id.devicename_item).setTitle(getString(R.string.menu_device_name) + " " + mDeviceName);
                }
            });
        }
    }

    @Override
    public void setPassword(String pass) {

        if (pass != null && !pass.equals("")) {
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.putString("password", pass);
            editor.commit();
            mPassword = pass;
        }
    }

    @Override
    public void setDebugMode(boolean status) {
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putBoolean("debugMode", status);
        editor.commit();
        mDebugMode = status;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                nvDrawer.getMenu().findItem(R.id.debug_mode_item).setTitle(getString(R.string.debug_mode_title) + " " + mDebugMode);
                if (mDebugMode) {
                    toolbar.getMenu().findItem(R.id.clear_btn).setVisible(true);
                    debugTv.setVisibility(View.VISIBLE);
                } else {
                    toolbar.getMenu().findItem(R.id.clear_btn).setVisible(false);
                    debugTv.setVisibility(View.GONE);
                }
            }
        });
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
            } else if (BluetoothEvents.BT_EVENT_SCAN_END.equals(action)) {

                Log.v(TAG, "Scan has ended");
                if (state == ButtonPusherState.DEVICE_FOUND) {
                    state = ButtonPusherState.CONNECT;
                    appendDebugTv("Connecting to device " + mDeviceAdress + " ...");
                    mService.connect(mDeviceAdress);
                }
            } else if (BluetoothEvents.BT_EVENT_DEVICE_DISCOVERED.equals(action)) {

                if (state == ButtonPusherState.SCAN) {
                    Log.v(TAG, "New device has been discovered");
                    final BluetoothObject btDeviceTmp = BluetoothObject.parseArrayList(intent);

                    if (btDeviceTmp != null && btDeviceTmp.getDeviceName().equals(mDeviceName)) {
                        Log.v(TAG, "found device " + mDeviceName);
                        state = ButtonPusherState.DEVICE_FOUND;
                        mDeviceAdress = btDeviceTmp.getDeviceAddress();
                        mService.stopScan();
                    }
                }

            } else if (BluetoothEvents.BT_EVENT_DEVICE_NOTIFICATION.equals(action)) {

                Log.v(TAG, "Device notification");
                if (state == ButtonPusherState.SEND_COMMAND) {
                    mService.disconnect(mDeviceAdress);
                    ArrayList<String> actionsStr = intent.getStringArrayListExtra("");
                    Log.v(TAG, "Receive notification : " + actionsStr.get(0));
                    appendDebugTv("Receive notification : " + actionsStr.get(0) + " ...");
                    state = ButtonPusherState.NONE;
                }
            } else if (BluetoothEvents.BT_EVENT_DEVICE_DISCONNECTED.equals(action)) {

                Log.v(TAG, "Device disconnected");
                if (state == ButtonPusherState.CONNECT) {
                    appendDebugTv("Error connection closed ...");
                    state = ButtonPusherState.NONE;
                }
            } else if (BluetoothEvents.BT_EVENT_DEVICE_CONNECTED.equals(action)) {

                Log.v(TAG, "device has been connected");

                if (state == ButtonPusherState.CONNECT) {

                    state = ButtonPusherState.SEND_COMMAND;

                    appendDebugTv("Sending command ...");
                    if (mService.getConnectionList().get(mDeviceAdress).getDevice() instanceof IRfduinoDevice) {
                        IRfduinoDevice device = (IRfduinoDevice) mService.getConnectionList().get(mDeviceAdress).getDevice();
                        device.sendPush(mPassword, new IPushListener() {
                            @Override
                            public void onPushFailure() {
                                appendDebugTv("Command failure...");
                            }

                            @Override
                            public void onPushSuccess() {
                                appendDebugTv("Command success...");
                            }
                        });
                    }
                }
            }
        }
    };

    private void clearReplaceDebugTv(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                debugTv.setText(text + System.getProperty("line.separator"));
            }
        });
    }

    private void appendDebugTv(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                debugTv.append(text + System.getProperty("line.separator"));
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
        return intentFilter;
    }
}
