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
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.ICharacteristicListener;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.IDeviceInitListener;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.IPushListener;
import com.github.akinaru.roboticbuttonpusher.model.TransmitState;
import com.github.akinaru.roboticbuttonpusher.service.BtPusherService;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

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
    private byte[] data;
    private int chunkNum;

    private final static int SENDING_BUFFER_MAX_LENGTH = 18;

    private int sendIndex = 0;
    private int sendingNum = 0;
    private boolean remain = false;
    private byte[] bitmapData;

    private boolean stopProcessingBitmap = true;
    private long dateProcessBegin = 0;
    private int failCount = 0;
    private int frameNumToSend = 0;

    private byte[] key = new byte[]{
            (byte) 0xF2, (byte) 0x1E, (byte) 0x07, (byte) 0x8C, (byte) 0x96, (byte) 0x99, (byte) 0x5E, (byte) 0xF7,
            (byte) 0xED, (byte) 0xF0, (byte) 0x91, (byte) 0x84, (byte) 0x06, (byte) 0x06, (byte) 0xF3, (byte) 0x94,
            (byte) 0x59, (byte) 0x90, (byte) 0x66, (byte) 0x63, (byte) 0x81, (byte) 0xE9, (byte) 0x14, (byte) 0x3E,
            (byte) 0x7B, (byte) 0x02, (byte) 0x7E, (byte) 0x08, (byte) 0xB6, (byte) 0xC7, (byte) 0x06, (byte) 0x26
    };

    private byte[] iv = new byte[]{
            (byte) 0xF7, (byte) 0x00, (byte) 0x15, (byte) 0x3C, (byte) 0x4E, (byte) 0x67, (byte) 0x34, (byte) 0xB9,
            (byte) 0xB0, (byte) 0x63, (byte) 0xCD, (byte) 0x7B, (byte) 0xD3, (byte) 0x8E, (byte) 0x79, (byte) 0xC5
    };

    private boolean init = false;

    /*
     * Creates a new pool of Thread objects for the download work queue
     */
    ExecutorService threadPool;

    /**
     * @param conn
     */
    @SuppressLint("NewApi")
    public RfduinoDevice(IBluetoothDeviceConn conn) {
        super(conn);

        threadPool = Executors.newFixedThreadPool(1);

        setCharacteristicListener(new ICharacteristicListener() {

            @Override
            public void onCharacteristicReadReceived(BluetoothGattCharacteristic charac) {
                Log.v(TAG, "onCharacteristicReadReceived");
            }

            @Override
            public void onCharacteristicChangeReceived(BluetoothGattCharacteristic charac) {

                Log.i(TAG, "receive something : " + charac.getUuid().toString() + " " + charac.getValue().length + " " + charac.getValue()[0]);

                if (charac.getUuid().toString().equals(RFDUINO_RECEIVE_CHARAC)) {

                    if (charac.getValue().length > 0) {

                        TransmitState state = TransmitState.getTransmitState(charac.getValue()[0]);

                        switch (state) {

                            case TRANSMIT_OK:
                                if (sendIndex != sendingNum) {
                                    Log.i(TAG, "received TRANSMIT_OK sending next batch of frames");

                                    threadPool.execute(new Runnable() {
                                        @Override
                                        public void run() {
                                            RfduinoDevice.this.conn.writeCharacteristic(RFDUINO_SERVICE, RFDUINO_SEND_CHARAC, new byte[]{(byte) TransmitState.TRANSMITTING.ordinal()}, new IPushListener() {
                                                @Override
                                                public void onPushFailure() {
                                                    Log.e(TAG, "error happenend setting bitmap length");
                                                }

                                                @Override
                                                public void onPushSuccess() {
                                                    Log.i(TAG, "set bitmap length successfull");
                                                    frameNumToSend = 128;
                                                    sendBitmapSequence();
                                                }
                                            });
                                        }
                                    });
                                } else {
                                    Log.i(TAG, "sending is over. Waiting for complete");
                                }
                                break;
                            case TRANSMIT_COMPLETE:
                                Log.i(TAG, "received TRANSMIT_COMPLETE");
                                clearBimapInfo();
                                break;
                        }
                    }
                }
            }

            @Override
            public void onCharacteristicWriteReceived(BluetoothGattCharacteristic charac) {
                Log.v(TAG, "onCharacteristicWriteReceived : " + new String(charac.getValue()));
            }
        });
    }

    private void clearBimapInfo() {
        sendingNum = 0;
        remain = false;
        sendIndex = 0;
        bitmapData = new byte[]{};
        stopProcessingBitmap = true;
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

        byte[] data = BtPusherService.encrypt(password);

        for (int i = 0; i < data.length; i++) {
            Log.i(TAG, "i:" + i + " " + (data[i] & 0xFF));
        }
        sendBitmap((byte) 0x00, BtPusherService.encrypt(password));
        /*
        try {
            byte[] res = new byte[64];
            byte[] clearText = new String("admin").getBytes();

            for (int i = 0; i < clearText.length; i++) {
                res[i] = clearText[i];
            }
            for (int i = clearText.length; i < 64; i++) {
                res[i] = 0;
            }
            for (int i = 0; i < res.length; i++) {
                System.out.println(res[i] & 0xFF);
            }

            byte[] data = AESCrypt.encrypt(new SecretKeySpec(key, "AES/CBC/NoPadding"), iv, res);

            for (int i = 0; i < data.length; i++) {
                Log.i(TAG, "i=" + i + " : " + "".format("0x%x", (data[i] & 0xFF)));
            }

            sendBitmap((byte) 0x00, data);


        } catch (GeneralSecurityException e) {
            e.printStackTrace();
        }
        */

        /*
        byte[] data = new byte[password.getBytes().length + 2];
        data[0] = 0;
        System.arraycopy(password.getBytes(), 0, data, 1, password.getBytes().length);
        data[data.length - 1] = '\0';
        getConn().writeCharacteristic(RFDUINO_SERVICE, RFDUINO_SEND_CHARAC, data, listener);
        */
    }

    private void sendBitmapSequence() {

        if (!stopProcessingBitmap) {

            if (sendIndex != sendingNum) {

                //Log.i(TAG, "index : " + sendIndex + " from " + (sendIndex * SENDING_BUFFER_MAX_LENGTH) + " to " + (sendIndex * SENDING_BUFFER_MAX_LENGTH + SENDING_BUFFER_MAX_LENGTH) + " with length of " + bitmapData.length);
                byte[] data = Arrays.copyOfRange(bitmapData, sendIndex * SENDING_BUFFER_MAX_LENGTH, sendIndex * SENDING_BUFFER_MAX_LENGTH + SENDING_BUFFER_MAX_LENGTH);
                sendIndex++;
                final long dateBegin = new Date().getTime();

                conn.writeCharacteristic(RFDUINO_SERVICE, RFDUINO_SEND_CHARAC, data, new IPushListener() {
                    @Override
                    public void onPushFailure() {

                        Log.e(TAG, "error happenend during transmission. Retrying");
 /*
                        sendIndex--;
                        failCount++;
                        sendBitmapSequence();
                        */
                    }

                    @Override
                    public void onPushSuccess() {
                        /*
                        long dateEnd = new Date().getTime();
                        float timeSpan = (dateEnd - dateBegin) / 1000f;
                        float speed = (SENDING_BUFFER_MAX_LENGTH * 8) / timeSpan;
                        Log.i(TAG, "current speed : " + speed + "bps");
                        */
                    }
                });
                frameNumToSend--;

                if (frameNumToSend != 0) {
                    sendBitmapSequence();
                }
            } else {

                if (remain) {

                    int remainNum = bitmapData.length % SENDING_BUFFER_MAX_LENGTH;
                    //Log.i(TAG, "index : " + sendingNum + " from " + (sendingNum * SENDING_BUFFER_MAX_LENGTH) + " to " + (sendingNum * SENDING_BUFFER_MAX_LENGTH + remainNum) + " with length of " + bitmapData.length);
                    byte[] data = Arrays.copyOfRange(bitmapData, sendingNum * SENDING_BUFFER_MAX_LENGTH, sendingNum * SENDING_BUFFER_MAX_LENGTH + remainNum);

                    conn.writeCharacteristic(RFDUINO_SERVICE, RFDUINO_SEND_CHARAC, data, new IPushListener() {
                        @Override
                        public void onPushFailure() {
                            /*
                            Log.e(TAG, "error happenend during transmission. Retrying");
                            failCount++;
                            sendBitmapSequence();
                            */
                        }

                        @Override
                        public void onPushSuccess() {
                            /*
                            Log.i(TAG, "completly finished in " + (new Date().getTime() - dateProcessBegin) + "ms - fail : " + failCount + " packet count : " + sendingNum);
                            clearBimapInfo();
                            */
                        }
                    });
                    Log.i(TAG, "completly finished in " + (new Date().getTime() - dateProcessBegin) + "ms - fail : " + failCount + " packet count : " + sendingNum);
                }
            }
        } else {
            Log.i(TAG, "stop processing bitmap");
        }
    }

    private void sendBitmap(byte cmd, final byte[] bitmapData) {
        Log.i(TAG, "send bitmap with length : " + bitmapData.length);

        sendingNum = bitmapData.length / SENDING_BUFFER_MAX_LENGTH;
        remain = false;
        if ((bitmapData.length % SENDING_BUFFER_MAX_LENGTH) != 0) {
            remain = true;
        }
        sendIndex = 0;
        RfduinoDevice.this.bitmapData = bitmapData;
        stopProcessingBitmap = false;
        dateProcessBegin = new Date().getTime();
        failCount = 0;

        //send cmd + length
        conn.writeCharacteristic(RFDUINO_SERVICE, RFDUINO_SEND_CHARAC, new byte[]{cmd, (byte) (bitmapData.length >> 8), (byte) bitmapData.length}, new IPushListener() {
            @Override
            public void onPushFailure() {
                Log.e(TAG, "error happenend setting bitmap length");
            }

            @Override
            public void onPushSuccess() {
                Log.i(TAG, "set bitmap length successfull");

            }
        });
        frameNumToSend = 127;
        sendBitmapSequence();
    }
}