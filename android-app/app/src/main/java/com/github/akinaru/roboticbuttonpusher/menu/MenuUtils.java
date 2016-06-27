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
package com.github.akinaru.roboticbuttonpusher.menu;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.support.v4.widget.DrawerLayout;
import android.view.MenuItem;

import com.github.akinaru.roboticbuttonpusher.R;
import com.github.akinaru.roboticbuttonpusher.dialog.AboutDialog;
import com.github.akinaru.roboticbuttonpusher.dialog.DeviceMessageDialog;
import com.github.akinaru.roboticbuttonpusher.dialog.DevicePasswordDialog;
import com.github.akinaru.roboticbuttonpusher.dialog.KeysDialog;
import com.github.akinaru.roboticbuttonpusher.dialog.OpenSourceItemsDialog;
import com.github.akinaru.roboticbuttonpusher.inter.IButtonPusher;

/**
 * Some functions used to manage Menu
 *
 * @author Bertrand Martel
 */
public class MenuUtils {

    /**
     * Execute actions according to selected menu item
     *
     * @param menuItem MenuItem object
     * @param mDrawer  navigation drawer
     * @param context  android context
     */
    public static void selectDrawerItem(MenuItem menuItem, DrawerLayout mDrawer, Context context, final IButtonPusher buttonPusher) {

        switch (menuItem.getItemId()) {
            case R.id.exit_item: {
                buttonPusher.disassociate();
                break;
            }
            case R.id.password_item: {
                if (buttonPusher != null) {
                    DevicePasswordDialog dialog = new DevicePasswordDialog(buttonPusher);
                    dialog.show();
                }
                break;
            }
            case R.id.keys_item: {
                if (buttonPusher != null) {
                    KeysDialog dialog = new KeysDialog(buttonPusher);
                    dialog.show();
                }
                break;
            }
            case R.id.message_item: {
                if (buttonPusher != null) {
                    DeviceMessageDialog dialog = new DeviceMessageDialog(buttonPusher);
                    dialog.show();
                }
                break;
            }
            case R.id.debug_mode_item: {

                CharSequence[] array = {"enabled", "disabled"};

                int indexCheck = buttonPusher.getDebugMode() ? 0 : 1;

                new AlertDialog.Builder(context)
                        .setSingleChoiceItems(array, indexCheck, null)
                        .setPositiveButton(buttonPusher.getContext().getResources().getString(R.string.dialog_ok), new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                dialog.dismiss();
                                int selectedPosition = ((AlertDialog) dialog).getListView().getCheckedItemPosition();
                                if (selectedPosition == 0) {
                                    buttonPusher.setDebugMode(true);
                                } else {
                                    buttonPusher.setDebugMode(false);
                                }
                            }
                        })
                        .setNegativeButton(buttonPusher.getContext().getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                dialog.dismiss();
                            }
                        })
                        .show();
                break;
            }
            case R.id.report_bugs: {
                Intent intent = new Intent(Intent.ACTION_SENDTO, Uri.fromParts(
                        "mailto", context.getResources().getString(R.string.email_addr), null));
                intent.putExtra(Intent.EXTRA_SUBJECT, context.getResources().getString(R.string.issue_subject));
                intent.putExtra(Intent.EXTRA_TEXT, context.getResources().getString(R.string.report_hint));
                context.startActivity(Intent.createChooser(intent, context.getResources().getString(R.string.issue_title)));
                break;
            }
            case R.id.open_source_components: {
                OpenSourceItemsDialog d = new OpenSourceItemsDialog(context);
                d.show();
                break;
            }
            case R.id.rate_app: {
                context.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + context.getApplicationContext().getPackageName())));
                break;
            }
            case R.id.about_app: {
                AboutDialog dialog = new AboutDialog(context);
                dialog.show();
                break;
            }
        }
        mDrawer.closeDrawers();
    }
}
