package com.github.akinaru.roboticbuttonpusher.inter;

/**
 * Created by akinaru on 09/06/16.
 */
public interface IInteractiveListener {

    void onSuccess();

    void onFailure();

    void onUserActionRequired();

    void onUserActionCommitted(String code);
}
