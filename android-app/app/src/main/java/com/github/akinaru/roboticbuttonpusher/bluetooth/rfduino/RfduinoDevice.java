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
package com.github.akinaru.roboticbuttonpusher.bluetooth.rfduino;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothGattCharacteristic;
import android.util.Log;

import com.github.akinaru.roboticbuttonpusher.bluetooth.connection.BluetoothDeviceAbstr;
import com.github.akinaru.roboticbuttonpusher.bluetooth.connection.IBluetoothDeviceConn;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothEvents;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.ICharacteristicListener;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.IDeviceInitListener;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.IPushListener;

import java.util.ArrayList;
import java.util.UUID;

/**
 * RFduino Bluetooth device management
 *
 * @author Bertrand Martel
 */
public class RfduinoDevice extends BluetoothDeviceAbstr implements IRfduinoDevice {

    private String TAG = RfduinoDevice.this.getClass().getName();

    public final static String RFDUINO_SERVICE = "00002220-0000-1000-8000-00805f9b34fb";
    public final static String RFDUINO_RECEIVE_CHARAC = "00002221-0000-1000-8000-00805f9b34fb";
    public final static String RFDUINO_SEND_CHARAC = "00002222-0000-1000-8000-00805f9b34fb";

    private ArrayList<IDeviceInitListener> initListenerList = new ArrayList<>();

    private boolean init = false;

    /**
     * @param conn
     */
    @SuppressLint("NewApi")
    public RfduinoDevice(IBluetoothDeviceConn conn) {
        super(conn);
        setCharacteristicListener(new ICharacteristicListener() {

            @Override
            public void onCharacteristicReadReceived(BluetoothGattCharacteristic charac) {
                Log.v(TAG, "onCharacteristicReadReceived");
            }

            @Override
            public void onCharacteristicChangeReceived(BluetoothGattCharacteristic charac) {
                Log.v(TAG, "onCharacteristicChangeReceived : " + new String(charac.getValue()));

                ArrayList<String> values = new ArrayList<>();
                values.add(new String(charac.getValue()));
                getConn().getManager().broadcastUpdateStringList(BluetoothEvents.BT_EVENT_DEVICE_NOTIFICATION, values);
            }

            @Override
            public void onCharacteristicWriteReceived(BluetoothGattCharacteristic charac) {
                Log.v(TAG, "onCharacteristicWriteReceived : " + new String(charac.getValue()));
            }
        });
    }

    @Override
    public void init() {

        Log.v(TAG, "initializing RFduino");

        conn.enableDisableNotification(UUID.fromString(RFDUINO_SERVICE), UUID.fromString(RFDUINO_RECEIVE_CHARAC), true);
        conn.enableGattNotifications(RFDUINO_SERVICE, RFDUINO_RECEIVE_CHARAC);

        for (int i = 0; i < initListenerList.size(); i++) {
            initListenerList.get(i).onInit();
        }
    }

    @Override
    public boolean isInit() {
        return init;
    }

    @Override
    public void addInitListener(IDeviceInitListener listener) {
        initListenerList.add(listener);
    }

    @Override
    public void sendPush(String password, IPushListener listener) {
        byte[] data = new byte[password.getBytes().length + 2];
        data[0] = 0;
        System.arraycopy(password.getBytes(), 0, data, 1, password.getBytes().length);
        data[data.length - 1] = '\0';
        getConn().writeCharacteristic(RFDUINO_SERVICE, RFDUINO_SEND_CHARAC, data, listener);
    }

}