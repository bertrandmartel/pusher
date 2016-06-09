package com.github.akinaru.roboticbuttonpusher.inter;

/**
 * Created by akinaru on 09/06/16.
 */
public interface IAssociateListener {

    void onAssociationSuccess();

    void onAssociationFailure();

    void onUserActionRequired();

    void onUserActionCommitted(String code);
}
