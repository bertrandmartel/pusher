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
package com.github.akinaru.roboticbuttonpusher.activity;

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

import com.github.akinaru.roboticbuttonpusher.R;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothObject;
import com.github.akinaru.roboticbuttonpusher.service.BtPusherService;

import java.text.SimpleDateFormat;


/**
 * Analyzer activity
 *
 * @author Bertrand Martel
 */
public class PushActivity extends BaseActivity {

    private String TAG = this.getClass().getName();

    /**
     * bluetooth device selected
     */
    private BluetoothObject btDevice = null;

    /**
     * date format for timestamp
     */
    private SimpleDateFormat sf = new SimpleDateFormat("HH:mm:ss.SSS");

    protected void onCreate(Bundle savedInstanceState) {

        setLayout(R.layout.activity_button_push);
        super.onCreate(savedInstanceState);

        //bind to service
        if (mBluetoothAdapter.isEnabled()) {
            Intent intent = new Intent(this, BtPusherService.class);
            mBound = bindService(intent, mServiceConnection, BIND_AUTO_CREATE);
        }
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

}
