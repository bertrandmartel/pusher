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
 * Transmition state.
 *
 * @author Bertrand Martel
 */
public enum TransmitState {

    TRANSMIT_NONE(0),
    TRANSMITTING(1),
    TRANSMIT_OK(2),
    TRANSMIT_COMPLETE(3);

    private int state;

    TransmitState(int state) {
        this.state = state;
    }

    public static TransmitState getTransmitState(byte data) {

        switch (data) {
            case 0:
                return TransmitState.TRANSMIT_NONE;
            case 1:
                return TransmitState.TRANSMITTING;
            case 2:
                return TransmitState.TRANSMIT_OK;
            case 3:
                return TransmitState.TRANSMIT_COMPLETE;
        }
        return TransmitState.TRANSMIT_NONE;
    }
}