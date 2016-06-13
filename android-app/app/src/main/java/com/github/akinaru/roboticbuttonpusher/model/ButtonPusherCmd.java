package com.github.akinaru.roboticbuttonpusher.model;

/**
 * Created by akinaru on 08/06/16.
 */
public enum ButtonPusherCmd {

    COMMAND_GET_TOKEN(0x00),
    COMMAND_ASSOCIATION_STATUS(0x01),
    COMMAND_PUSH(0x02),
    COMMAND_SET_PASSWORD(0x03),
    COMMAND_ASSOCIATE(0x04),
    COMMAND_SET_KEY(0x05),
    COMMAND_FAILURE(0x06),
    COMMAND_ASSOCIATE_RESPONSE(0x07),
    COMMAND_RECEIVE_KEYS(0x08),
    COMMAND_SET_PASSWORD_RESPONSE(0x09),
    COMMAND_SET_KEYS_RESPONSE(0x0A),
    COMMAND_DEASSOCIATE(0x0B),
    COMMAND_NONE(0xFF);

    private int mCode;

    private ButtonPusherCmd(int code) {
        mCode = code;
    }

    public static ButtonPusherCmd getValue(int value) {

        for (ButtonPusherCmd cmd : ButtonPusherCmd.values()) {
            if (value == cmd.mCode)
                return cmd;
        }
        return COMMAND_NONE;
    }
}
