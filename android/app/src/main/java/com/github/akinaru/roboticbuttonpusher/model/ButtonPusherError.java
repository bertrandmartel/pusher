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
 * Error type used.
 *
 * @author Bertrand Martel
 */
public enum ButtonPusherError {
    SCAN_TIMEOUT,
    CONNECTION_TIMEOUT,
    PUSH_FAILURE,
    SET_PASSWORD_TIMEOUT,
    SET_MESSAGE_TIMEOUT,
    NOTIFICATION_TIMEOUT,
    ASSOCIATION_FAILURE,
    SET_KEYS_TIMEOUT,
    SET_KEYS_FAILURE,
    SET_PASSWORD_FAILURE,
    BLUETOOTH_STATE_TIMEOUT,
    DISASSOCIATE_FAILURE,
    DISASSOCIATE_TIMEOUT,
    SET_MESSAGE_FAILURE
}
