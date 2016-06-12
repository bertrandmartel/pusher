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

import com.github.akinaru.roboticbuttonpusher.model.BtnPusherKeysType;

/**
 * Generic interface for Rfduino device
 *
 * @author Bertrand Martel
 */
public interface IRfduinoDevice {

    void sendPush(String password);

    void sendAssociationCode(String code);

    void setPassword(String password);

    void setKeys(String password, BtnPusherKeysType keysType);
}