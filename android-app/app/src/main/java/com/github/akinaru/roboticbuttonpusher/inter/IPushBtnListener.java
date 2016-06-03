package com.github.akinaru.roboticbuttonpusher.inter;

import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherError;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherState;

/**
 * Created by akinaru on 02/06/16.
 */
public interface IPushBtnListener {

    void onChangeState(ButtonPusherState newState);

    void onError(ButtonPusherError error);

}
