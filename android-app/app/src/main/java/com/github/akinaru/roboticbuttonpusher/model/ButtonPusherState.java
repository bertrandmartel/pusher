package com.github.akinaru.roboticbuttonpusher.model;

/**
 * Created by akinaru on 02/06/16.
 */
public enum ButtonPusherState {
    NONE,
    PROCESS_END,
    SCAN,
    DEVICE_FOUND,
    CONNECT,
    WAIT_DEVICE_START,
    WAIT_BLUETOOTH_STATE, SEND_COMMAND
}
