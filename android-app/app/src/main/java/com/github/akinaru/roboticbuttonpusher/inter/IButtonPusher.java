package com.github.akinaru.roboticbuttonpusher.inter;

import android.content.Context;

/**
 * Created by akinaru on 01/06/16.
 */
public interface IButtonPusher {

    String getPassword();

    String getDeviceName();

    boolean getDebugMode();

    Context getContext();

    void setDeviceName(String deviceName);

    void setPassword(String pass);

    void setDebugMode(boolean status);

    void uploadPassword(String pass);

    void sendAssociationCode(String code);

    void sendAssociationCodeFail();
}
