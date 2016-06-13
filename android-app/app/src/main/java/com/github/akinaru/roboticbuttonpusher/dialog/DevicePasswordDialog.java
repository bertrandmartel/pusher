/**************************************************************************
 * This file is part of HCI Debugger                                      *
 * <p/>                                                                   *
 * Copyright (C) 2016  Bertrand Martel                                    *
 * <p/>                                                                   *
 * Foobar is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 * <p/>                                                                   *
 * Foobar is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 * <p/>                                                                   *
 * You should have received a copy of the GNU General Public License      *
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.        *
 */
package com.github.akinaru.roboticbuttonpusher.dialog;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;

import com.github.akinaru.roboticbuttonpusher.R;
import com.github.akinaru.roboticbuttonpusher.inter.IButtonPusher;

/**
 * Max packet count dialog
 *
 * @author Bertrand Martel
 */
public class DevicePasswordDialog extends AlertDialog {

    private String oldPass;

    public DevicePasswordDialog(final IButtonPusher activity) {
        super(activity.getContext());

        LayoutInflater inflater = getLayoutInflater();
        View dialoglayout = inflater.inflate(R.layout.device_password_dialog, null);
        setView(dialoglayout);

        final EditText devicePassEt = (EditText) dialoglayout.findViewById(R.id.device_password_value);
        oldPass = activity.getPassword();
        devicePassEt.setText(oldPass);

        final CheckBox checkbox = (CheckBox) dialoglayout.findViewById(R.id.device_password_display_cb);

        checkbox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (!isChecked) {
                    devicePassEt.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
                } else {
                    devicePassEt.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
                }
            }
        });

        final CheckBox passwordUploadCb = (CheckBox) dialoglayout.findViewById(R.id.device_password_set_ondevice_cb);

        checkbox.setChecked(true);
        passwordUploadCb.setChecked(false);

        setTitle(R.string.rfduino_device_password);
        setButton(DialogInterface.BUTTON_POSITIVE, activity.getContext().getResources().getString(R.string.dialog_ok), new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                activity.setPassword(devicePassEt.getText().toString());
                if (passwordUploadCb.isChecked()) {
                    activity.uploadPassword(oldPass);
                }
            }
        });

        setButton(DialogInterface.BUTTON_NEGATIVE, activity.getContext().getResources().getString(R.string.cancel), new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {

            }
        });
    }
}