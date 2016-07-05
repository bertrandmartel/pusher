/**********************************************************************************
 * This file is part of Pusher.                                                    *
 * <p/>                                                                            *
 * Copyright (C) 2016  Bertrand Martel                                             *
 * <p/>                                                                            *
 * Pusher is free software: you can redistribute it and/or modify                  *
 * it under the terms of the GNU General Public License as published by            *
 * the Free Software Foundation, either version 3 of the License, or               *
 * (at your option) any later version.                                             *
 * <p/>                                                                            *
 * Pusher is distributed in the hope that it will be useful,                       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   *
 * GNU General Public License for more details.                                    *
 * <p/>                                                                            *
 * You should have received a copy of the GNU General Public License               *
 * along with Pusher. If not, see <http://www.gnu.org/licenses/>.                  *
 */
package com.github.akinaru.roboticbuttonpusher.inter;

import android.content.Context;

/**
 * @author Bertrand Martel
 */
public interface IButtonPusher {

    /**
     * get device password.
     *
     * @return
     */
    String getPassword();

    /**
     * get debug mode.
     *
     * @return
     */
    boolean getDebugMode();

    /**
     * get Android context.
     *
     * @return
     */
    Context getContext();

    /**
     * set device password.
     *
     * @param pass
     */
    void setPassword(String pass);

    /**
     * set debug mode.
     *
     * @param status
     */
    void setDebugMode(boolean status);

    /**
     * set a new password to the device.
     *
     * @param pass
     */
    void uploadPassword(String pass);

    /**
     * called to notify user to enter association code.
     *
     * @param code
     */
    void sendAssociationCode(String code);

    /**
     * send association failure notification.
     */
    void sendAssociationCodeFail();

    /**
     * generate new aes key for the device.
     */
    void generateNewAesKey();

    /**
     * disassociate this device.
     */
    void disassociate();

    /**
     * get last lcd top message entered by user.
     *
     * @return
     */
    String getTopMessage();

    /**
     * get last lcd bottom message entered by user.
     *
     * @return
     */
    String getBotttomMessage();

    /**
     * set lcd message on the device.
     *
     * @param topMessage
     * @param bottomMessage
     */
    void setMessage(String topMessage, String bottomMessage);

    /**
     * test if user accepted location permission
     *
     * @return
     */
    boolean giveUpNoPermission();

    /**
     * request the location permission
     */
    void requestPermission();

}
