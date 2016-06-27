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
package com.github.akinaru.roboticbuttonpusher.bluetooth;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.util.Log;

import com.github.akinaru.roboticbuttonpusher.bluetooth.connection.BluetoothDeviceConn;
import com.github.akinaru.roboticbuttonpusher.bluetooth.connection.IBluetoothDeviceConn;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothEvents;
import com.github.akinaru.roboticbuttonpusher.bluetooth.listener.IPushListener;
import com.github.akinaru.roboticbuttonpusher.constant.JsonConstants;
import com.github.akinaru.roboticbuttonpusher.service.BtPusherService;
import com.github.akinaru.roboticbuttonpusher.utils.ManualResetEvent;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;


/**
 * Bluetooth android API processing : contains all android bluetooth api
 * <p/>
 * alternative to this is using an Android Service that you can bind to your main activity
 *
 * @author Bertrand Martel
 */
public class BluetoothCustomManager implements IBluetoothCustomManager {

    private final static String TAG = BluetoothCustomManager.class.getName();

    // set init pool size
    private static final int CORE_POOL_SIZE = 1;

    // set max pool size
    private static final int MAXIMUM_POOL_SIZE = 1;

    // Sets the amount of time an idle thread will wait for a task before terminating
    private static final int KEEP_ALIVE_TIME = 5;

    // set time unit in seconds
    private static final TimeUnit KEEP_ALIVE_TIME_UNIT = TimeUnit.SECONDS;

    LinkedBlockingQueue gattWorkingQueue = new LinkedBlockingQueue<Runnable>();

    /*
     * Creates a new pool of Thread objects for the download work queue
     */
    ThreadPoolExecutor gattThreadPool = new ThreadPoolExecutor(CORE_POOL_SIZE, MAXIMUM_POOL_SIZE,
            KEEP_ALIVE_TIME, KEEP_ALIVE_TIME_UNIT, gattWorkingQueue);

    /**
     * timeout for waiting for response frame from the device
     */
    private final static int BT_TIMEOUT = 2000;

    /**
     * set bluetooth scan period
     */
    private final int SCAN_PERIOD = 30000;

    /**
     * list of bluetooth connection by address
     */
    private HashMap<String, IBluetoothDeviceConn> bluetoothConnectionList = new HashMap<>();

    private HashMap<String, BluetoothDevice> scanningList = new HashMap<>();

    /**
     * event manager used to block / release process
     */
    private ManualResetEvent eventManager = new ManualResetEvent(false);

    /**
     * Bluetooth adapter
     */
    private BluetoothAdapter mBluetoothAdapter = null;

    /**
     * message handler
     */
    private Handler mHandler = null;

    /**
     * set bluetooth scan
     */
    private volatile boolean scanning = false;

    /**
     * Callback for Bluetooth adapter
     * This will be called when a bluetooth device has been discovered
     */
    private BluetoothAdapter.LeScanCallback scanCallback = null;

    private Context context = null;

    private ScheduledExecutorService executor = Executors.newScheduledThreadPool(1);

    private HashMap<String, ScheduledFuture<?>> waitingForDisconnectionList = new HashMap<>();

    private BtPusherService service;

    /**
     * Build bluetooth manager
     */
    public BluetoothCustomManager(BtPusherService service) {
        this.service = service;
        this.context = service.getBaseContext();
    }

    @SuppressLint("NewApi")
    public void init(Context context) {

        // Initializes Bluetooth adapter.
        final BluetoothManager bluetoothManager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);

        mBluetoothAdapter = bluetoothManager.getAdapter();

        //init message handler
        mHandler = null;
        mHandler = new Handler();

        scanCallback = new BluetoothAdapter.LeScanCallback() {

            @Override
            public void onLeScan(BluetoothDevice device, int rssi, final byte[] scanRecord) {

                if (device.getAddress() != null &&
                        device.getName() != null) {

                    if (device.getName().equals("RFdroid")) {
                        dispatchRFdroid(device, rssi, scanRecord);
                    } else {
                        dispatchBtDevices(device, rssi, scanRecord);
                    }
                }
            }
        };
    }

    private void dispatchRFdroid(BluetoothDevice device, int rssi, final byte[] scanRecord) {

        if (scanningList.containsKey(device.getAddress())) {

        } else {
            Log.i(TAG, "found a RFdroid");
        }
    }

    private void dispatchBtDevices(BluetoothDevice device, int rssi, final byte[] scanRecord) {

        if (scanningList.containsKey(device.getAddress())) {

        } else {

            Log.i(TAG, "found a new Bluetooth device : " + device.getName() + " : " + device.getAddress());

            scanningList.put(device.getAddress(), device);

            try {
                JSONObject object = new JSONObject();
                object.put(JsonConstants.BT_ADDRESS, device.getAddress());
                object.put(JsonConstants.BT_DEVICE_NAME, device.getName());
                object.put(JsonConstants.BT_ADVERTISING_INTERVAL, -1);

                ArrayList<String> deviceInfo = new ArrayList<>();
                deviceInfo.add(object.toString());

                broadcastUpdateStringList(BluetoothEvents.BT_EVENT_DEVICE_DISCOVERED, deviceInfo);

            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * clear list adapter (usually before rescanning)
     */
    public void clearScanningList() {
        scanningList.clear();
    }

    /**
     * Scan new Bluetooth device
     */
    @SuppressLint("NewApi")
    public boolean scanLeDevice() {

        if (!scanning) {

            broadcastUpdate(BluetoothEvents.BT_EVENT_SCAN_START);

            scanning = true;

            return mBluetoothAdapter.startLeScan(scanCallback);
        }
        return false;
    }

    /**
     * Stop Bluetooth LE scanning
     */
    @SuppressLint("NewApi")
    public void stopScan() {
        mHandler.removeCallbacksAndMessages(null);
        scanning = false;
        mBluetoothAdapter.stopLeScan(scanCallback);
        //notify end of scan
        broadcastUpdate(BluetoothEvents.BT_EVENT_SCAN_END);
    }

    public boolean isScanning() {
        return scanning;
    }

    /**
     * Connect to device's GATT server
     */
    @SuppressLint("NewApi")
    public boolean connect(String address) {

        if (mBluetoothAdapter == null || address == null) {
            Log.w(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);

        boolean alreadyInList = false;

        if (bluetoothConnectionList.containsKey(address)) {
            alreadyInList = true;
        }

        if (alreadyInList) {

            Log.i(TAG, "reusing same connection");

            BluetoothDeviceConn conn = (BluetoothDeviceConn) bluetoothConnectionList.get(address);

            conn.setGatt(device.connectGatt(context, false, conn.getGattCallback()));

        } else {

            BluetoothDeviceConn conn = new BluetoothDeviceConn(address, device.getName(), this);

            bluetoothConnectionList.put(address, conn);

            Log.i(TAG, "new connection");
            //connect to gatt server on the device
            conn.setGatt(device.connectGatt(context, false, conn.getGattCallback()));
        }

        return true;
    }

    @Override
    public ManualResetEvent getEventManager() {
        return eventManager;
    }

    /**
     * Send broadcast data through broadcast receiver
     *
     * @param action action to be sent
     */
    @Override
    public void broadcastUpdate(final String action) {
        final Intent intent = new Intent(action);
        context.sendBroadcast(intent);
    }

    /**
     * broadcast characteristic value
     *
     * @param action action to be sent (data available)
     */
    @Override
    public void broadcastUpdateStringList(String action, ArrayList<String> valueList) {

        String valueName = "";
        final Intent intent = new Intent(action);
        intent.putStringArrayListExtra(valueName, valueList);
        context.sendBroadcast(intent);
    }

    @SuppressLint("NewApi")
    @Override
    public void writeCharacteristic(String characUid, byte[] value, BluetoothGatt gatt, IPushListener listener) {

        if (gatt != null && characUid != null && value != null) {

            gattThreadPool.execute(new GattTask(gatt, characUid, value, listener) {
                @Override
                public void run() {

                    BluetoothGattCharacteristic charac = GattUtils.getCharacteristic(getGatt().getServices(), getUid());
                    //charac.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
                    charac.setValue(getValue());

                    getGatt().writeCharacteristic(charac);

                    long startTime = System.currentTimeMillis();
                    eventManager.reset();
                    try {
                        eventManager.waitOne(BT_TIMEOUT);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    long endTime = System.currentTimeMillis();

                    if ((endTime - startTime) >= BT_TIMEOUT) {
                        if (getListener() != null) {
                            getListener().onPushFailure();
                        }
                    } else {
                        if (getListener() != null) {
                            getListener().onPushSuccess();
                        }
                    }
                }
            });
            gattThreadPool.execute(new Runnable() {
                @Override
                public void run() {


                }
            });
        } else
            Log.e(TAG, "Error int writeCharacteristic() input argument NULL");
    }

    @SuppressLint("NewApi")
    @Override
    public void readCharacteristic(String characUid, BluetoothGatt gatt) {

        if (gatt != null && characUid != null) {

            gattThreadPool.execute(new GattTask(gatt, characUid, null, null) {
                @Override
                public void run() {

                    BluetoothGattCharacteristic charac = GattUtils.getCharacteristic(getGatt().getServices(), getUid());

                    getGatt().readCharacteristic(charac);
                    eventManager.reset();
                    try {
                        eventManager.waitOne(BT_TIMEOUT);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            });
        } else
            Log.e(TAG, "Error int writeCharacteristic() input argument NULL");
    }

    @SuppressLint("NewApi")
    @Override
    public void writeDescriptor(String descriptorUid, BluetoothGatt gatt, byte[] value, String serviceUid, String characUid) {

        if (gatt != null && descriptorUid != null) {

            gattThreadPool.execute(new GattTask(gatt, descriptorUid, value, serviceUid, characUid) {
                @Override
                public void run() {

                    BluetoothGattDescriptor descriptor = getGatt().getService(UUID.fromString(getDescriptorServiceUid()))
                            .getCharacteristic(UUID.fromString(getDescriptorCharacUid())).getDescriptor(UUID.fromString(getUid()));

                    descriptor.setValue(getValue());

                    getGatt().writeDescriptor(descriptor);
                    eventManager.reset();
                    try {
                        eventManager.waitOne(BT_TIMEOUT);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                }
            });
            gattThreadPool.execute(new Runnable() {
                @Override
                public void run() {

                }
            });
        } else
            Log.e(TAG, "Error int writeCharacteristic() input argument NULL");
    }

    @Override
    public HashMap<String, IBluetoothDeviceConn> getConnectionList() {
        return bluetoothConnectionList;
    }

    @Override
    public HashMap<String, ScheduledFuture<?>> getWaitingMap() {
        return waitingForDisconnectionList;
    }

    @SuppressLint("NewApi")
    public boolean disconnectDevice(final String deviceAddress, final boolean remove) {

        if (mBluetoothAdapter == null || deviceAddress == null) {
            Log.w(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

        if (bluetoothConnectionList.containsKey(deviceAddress)) {

            if (bluetoothConnectionList.get(deviceAddress).getBluetoothGatt() != null && bluetoothConnectionList.get(deviceAddress).isConnected()) {
                Log.i(TAG, "disconnect device");
                bluetoothConnectionList.get(deviceAddress).getBluetoothGatt().disconnect();

                if (!waitingForDisconnectionList.containsKey(deviceAddress)) {

                    ScheduledFuture<?> task = executor.schedule(new Runnable() {
                        @Override
                        public void run() {
                            Log.i(TAG, "connection forced close");
                            bluetoothConnectionList.get(deviceAddress).getBluetoothGatt().close();
                            waitingForDisconnectionList.remove(deviceAddress);

                            if (remove) {
                                Log.i(TAG, "removing device ...");
                                bluetoothConnectionList.remove(deviceAddress);
                                try {
                                    Thread.sleep(500);
                                } catch (InterruptedException e) {
                                    e.printStackTrace();
                                }
                                broadcastUpdateStringList(BluetoothEvents.BT_EVENT_DEVICE_REMOVED, new ArrayList<String>());
                            }
                        }
                    }, 1000, TimeUnit.MILLISECONDS);

                    waitingForDisconnectionList.put(deviceAddress, task);
                }

                bluetoothConnectionList.get(deviceAddress).setConnected(false);

            } else {

                Log.i(TAG, "removing not connected device");

                if (remove) {
                    Log.i(TAG, "removing device ...");
                    bluetoothConnectionList.remove(deviceAddress);
                    waitingForDisconnectionList.remove(deviceAddress);
                    return false;
                }
            }
            return true;
        } else {
            Log.e(TAG, "device " + deviceAddress + " not found in list");
        }
        return false;
    }

    @SuppressLint("NewApi")
    public boolean disconnect(final String deviceAddress) {
        return disconnectDevice(deviceAddress, false);
    }

    public boolean disconnectAndRemove(final String deviceAddress) {
        return disconnectDevice(deviceAddress, true);
    }

    public void disconnectAll() {
        Iterator it = getConnectionList().entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry<String, IBluetoothDeviceConn> pair = (Map.Entry) it.next();
            pair.getValue().disconnect();
        }
    }

    public HashMap<String, BluetoothDevice> getScanningList() {
        return scanningList;
    }

    public BtPusherService getService() {
        return service;
    }
}
