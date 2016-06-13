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
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;

import com.github.akinaru.roboticbuttonpusher.R;
import com.github.akinaru.roboticbuttonpusher.inter.IButtonPusher;

/**
 * Max packet count dialog
 *
 * @author Bertrand Martel
 */
public class KeysDialog extends AlertDialog {

    private String TAG = KeysDialog.class.getSimpleName();

    public KeysDialog(final IButtonPusher activity) {
        super(activity.getContext());

        LayoutInflater inflater = getLayoutInflater();
        final View dialoglayout = inflater.inflate(R.layout.device_key_dialog, null);
        setView(dialoglayout);

        setTitle(R.string.rfduino_device_keys);

        Button generate_key_btn = (Button) dialoglayout.findViewById(R.id.generate_key_btn);
        Button default_key_btn = (Button) dialoglayout.findViewById(R.id.default_key_btn);

        generate_key_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.i(TAG, "generate key");
                activity.generateNewAesKey();
                cancel();
            }
        });

        default_key_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.i(TAG, "default key");
                activity.generateDefaultAesKey();
                cancel();
            }
        });

        setButton(DialogInterface.BUTTON_POSITIVE, activity.getContext().getResources().getString(R.string.dialog_ok), new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {

            }
        });

        setButton(DialogInterface.BUTTON_NEGATIVE, activity.getContext().getResources().getString(R.string.cancel), new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {

            }
        });
    }
}