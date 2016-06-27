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
package com.github.akinaru.roboticbuttonpusher.model;

/**
 * Command used by the device.
 *
 * @author Bertrand Martel
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
    COMMAND_MESSAGE_TOP(0x0C),
    COMMAND_MESSAGE_BOTTOM(0x0D),
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
