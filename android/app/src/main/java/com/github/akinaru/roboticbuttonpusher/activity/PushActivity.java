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
package com.github.akinaru.roboticbuttonpusher.activity;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.support.design.widget.FloatingActionButton;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.Toast;

import com.github.akinaru.roboticbuttonpusher.PushSingleton;
import com.github.akinaru.roboticbuttonpusher.R;
import com.github.akinaru.roboticbuttonpusher.bluetooth.events.BluetoothEvents;
import com.github.akinaru.roboticbuttonpusher.constant.SharedPrefConst;
import com.github.akinaru.roboticbuttonpusher.dialog.UserActionDialog;
import com.github.akinaru.roboticbuttonpusher.inter.IPushBtnListener;
import com.github.akinaru.roboticbuttonpusher.inter.ISingletonListener;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherError;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherState;
import com.github.silvestrpredko.dotprogressbar.DotProgressBar;

/**
 * Main Button Push activity
 *
 * @author Bertrand Martel
 */
public class PushActivity extends BaseActivity implements ISingletonListener {

    private String TAG = this.getClass().getName();

    private static final boolean DEFAULT_DEBUG_MODE = false;

    private String mPassword;

    private boolean mDebugMode;

    private FloatingActionButton button;
    private FloatingActionButton buttonAssociated;

    private PushSingleton mSingleton;

    /**
     * shared preference object.
     */
    private SharedPreferences sharedPref;

    private static final int REQUEST_PERMISSION_COARSE_LOCATION = 2;

    private UserActionDialog dialog;

    protected void onCreate(Bundle savedInstanceState) {

        setLayout(R.layout.activity_button_push);
        super.onCreate(savedInstanceState);

        Log.v(TAG, "oncreate");

        //register bluetooth event broadcast receiver
        registerReceiver(mBluetoothReceiver, makeGattUpdateIntentFilter());

        mAnimationScaleUp = AnimationUtils.loadAnimation(this, R.anim.scale_up);
        mAnimationScaleDown = AnimationUtils.loadAnimation(this, R.anim.scale_down);
        mAnimationDefaultScaleUp = AnimationUtils.loadAnimation(this, R.anim.scale_default_up);

        dotProgressBar = (DotProgressBar) findViewById(R.id.dot_progress_bar);
        mFailureButton = (FloatingActionButton) findViewById(R.id.failure_button);

        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "your device has no BLE feature", Toast.LENGTH_LONG).show();
            finish();
        }

        mAnimationScaleUp.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation arg0) {
            }

            @Override
            public void onAnimationRepeat(Animation arg0) {
            }

            @Override
            public void onAnimationEnd(Animation arg0) {

                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if (mFailure) {
                                    mFailureButton.startAnimation(mAnimationScaleDown);
                                } else {
                                    mImgSelection.startAnimation(mAnimationScaleDown);
                                }
                            }
                        });

                    }
                }, 1000);

            }
        });

        mAnimationScaleDown.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation arg0) {
            }

            @Override
            public void onAnimationRepeat(Animation arg0) {
            }

            @Override
            public void onAnimationEnd(Animation arg0) {

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mFailure) {
                            mFailure = false;
                            mFailureButton.setVisibility(View.GONE);
                        } else {
                            mImgSelection.setVisibility(View.GONE);
                        }
                        if (!mAssociated) {
                            button.setVisibility(View.VISIBLE);
                            button.startAnimation(mAnimationDefaultScaleUp);
                        } else {
                            buttonAssociated.setVisibility(View.VISIBLE);
                            buttonAssociated.setVisibility(View.VISIBLE);
                            buttonAssociated.startAnimation(mAnimationDefaultScaleUp);
                        }
                    }
                });

            }
        });

        //shared preference
        sharedPref = getSharedPreferences(SharedPrefConst.PREFERENCES, Context.MODE_PRIVATE);
        mPassword = sharedPref.getString(SharedPrefConst.DEVICE_PASSWORD_FIELD, SharedPrefConst.DEFAULT_PASSWORD);
        mDebugMode = sharedPref.getBoolean(SharedPrefConst.DEBUG_MODE_FIELD, DEFAULT_DEBUG_MODE);
        mAssociated = sharedPref.getBoolean(SharedPrefConst.ASSOCIATED_STATUS, false);
        mImgSelection = (FloatingActionButton) findViewById(R.id.img_selection);

        mSingleton = PushSingleton.getInstance();
        mSingleton.unbindService(this);
        mSingleton.setSingletonListener(this);
        mSingleton.setAssociate(true);

        if (mAssociated) {
            findViewById(R.id.fab).setVisibility(View.GONE);
            findViewById(R.id.fab_associated).setVisibility(View.VISIBLE);
        }

        if (mDisassociateMenuItem != null) {
            if (!mAssociated) {
                mDisassociateMenuItem.setVisible(false);
            }
        }

        button = (FloatingActionButton) findViewById(R.id.fab);
        buttonAssociated = (FloatingActionButton) findViewById(R.id.fab_associated);

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (giveUpNoPermission()) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            button.setVisibility(View.GONE);
                            dotProgressBar.setVisibility(View.VISIBLE);
                            dotProgressBar.setAlpha(1);
                        }
                    });

                    clearReplaceDebugTv("Scanning for device ...");

                    mSingleton.startPushTask();
                } else {
                    requestPermission();
                }
            }
        });

        buttonAssociated.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (giveUpNoPermission()) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            buttonAssociated.setVisibility(View.GONE);
                            dotProgressBar.setVisibility(View.VISIBLE);
                            dotProgressBar.setAlpha(1);
                        }
                    });

                    clearReplaceDebugTv("Scanning for device ...");

                    mSingleton.startPushTask();
                } else {
                    requestPermission();
                }
            }
        });

        if (Build.VERSION.SDK_INT >= 23) {
            if (checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                Log.v(TAG, "requesting location permission");
                requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_PERMISSION_COARSE_LOCATION);
            }
        }
        mSingleton.bindService(getApplicationContext());
    }

    private void showPermissionRequired() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(PushActivity.this, "location permissions is required for BLE scan", Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_PERMISSION_COARSE_LOCATION: {
                if (grantResults.length > 0 && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                    showPermissionRequired();
                }
            }
        }
    }

    private IPushBtnListener mBtnListener = new IPushBtnListener() {
        @Override
        public void onChangeState(ButtonPusherState newState) {

            switch (newState) {
                case SCAN:
                    break;
                case DEVICE_FOUND:
                    break;
                case CONNECT:
                    appendDebugTv("Connecting to device ...");
                    break;
                case SEND_COMMAND:
                    appendDebugTv("Sending command ...");
                    break;
                case PROCESS_END:
                    appendDebugTv("Receive command success ...");

                    if (mImgSelection != null && button != null) {

                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                button.setVisibility(View.GONE);
                                dotProgressBar.setVisibility(View.GONE);
                                mImgSelection.setVisibility(View.VISIBLE);
                                mImgSelection.clearAnimation();
                                mImgSelection.startAnimation(mAnimationScaleUp);
                            }
                        });

                    }
                    break;
                default:
                    break;
            }
        }

        @Override
        public void onError(ButtonPusherError error) {
            switch (error) {
                case SCAN_TIMEOUT:
                    appendDebugTv("Error, device not found");
                    break;
                case CONNECTION_TIMEOUT:
                    appendDebugTv("Error, connection failed");
                    break;
                case PUSH_FAILURE:
                    appendDebugTv("Command failure...");
                    break;
                case NOTIFICATION_TIMEOUT:
                    break;
                case SET_MESSAGE_FAILURE:
                    appendDebugTv("set message failure");
                    break;
                case SET_MESSAGE_TIMEOUT:
                    appendDebugTv("set message timeout...");
                    break;
                case BLUETOOTH_STATE_TIMEOUT:
                    appendDebugTv("Bluetooth state timeout...");
                    break;
                default:
                    break;
            }
            showFailure();
        }
    };

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        boolean ret = super.onPrepareOptionsMenu(menu);
        String debugModeStr = mDebugMode ? "enabled" : "disabled";

        nvDrawer.getMenu().findItem(R.id.debug_mode_item).setTitle(getString(R.string.debug_mode_title) + " " + debugModeStr);
        if (mDebugMode) {
            toolbar.getMenu().findItem(R.id.clear_btn).setVisible(true);
            debugTv.setVisibility(View.VISIBLE);
        }
        return ret;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mBluetoothReceiver);
        mSingleton.stopPushTask();
        mSingleton.unbindService(getApplicationContext());
        Log.v(TAG, "onDestroy");
    }

    @Override
    public boolean giveUpNoPermission() {
        if (Build.VERSION.SDK_INT >= 23) {
            if (checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    @Override
    public void requestPermission() {
        if (Build.VERSION.SDK_INT >= 23) {
            requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_PERMISSION_COARSE_LOCATION);
        }
    }

    /**
     * broadcast receiver to receive bluetooth events
     */
    private final BroadcastReceiver mBluetoothReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {

            final String action = intent.getAction();

            if (BluetoothEvents.BT_EVENT_DEVICE_USER_ACTION_REQUIRED.equals(action)) {
                Log.v(TAG, "show user action dialog");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        dialog = new UserActionDialog(PushActivity.this);
                        dialog.show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_DISCONNECTED.equals(action)) {
                Log.v(TAG, "device has been disconnected");
                if (dialog != null) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            dialog.dismiss();
                        }
                    });
                }
            } else if (BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_SUCCESS.equals(action)) {
                Log.v(TAG, "association success");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mDisassociateMenuItem != null) {
                            mDisassociateMenuItem.setVisible(true);
                        }
                        mAssociated = true;
                        SharedPreferences.Editor editor = sharedPref.edit();
                        editor.putBoolean(SharedPrefConst.ASSOCIATED_STATUS, mAssociated);
                        editor.commit();
                        Toast.makeText(PushActivity.this, "association success", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_FAILURE.equals(action)) {
                Log.v(TAG, "association failure");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(PushActivity.this, "association failure", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_SUCCESS.equals(action)) {
                Log.v(TAG, "set keys success");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mAssociated = false;
                        findViewById(R.id.fab).setVisibility(View.VISIBLE);
                        findViewById(R.id.fab_associated).setVisibility(View.GONE);
                        SharedPreferences.Editor editor = sharedPref.edit();
                        editor.putBoolean(SharedPrefConst.ASSOCIATED_STATUS, mAssociated);
                        editor.commit();
                        Toast.makeText(PushActivity.this, "set key success", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_FAILURE.equals(action)) {
                Log.v(TAG, "set keys failure");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(PushActivity.this, "set key failure", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_SUCCESS.equals(action)) {
                Log.v(TAG, "set password success");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(PushActivity.this, "set password success", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_FAILURE.equals(action)) {
                Log.v(TAG, "set password failure");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(PushActivity.this, "set password failure", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_FAILURE.equals(action)) {
                Log.v(TAG, "deassociate failure");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mDisassociateMenuItem != null) {
                            mDisassociateMenuItem.setVisible(true);
                        }
                        mAssociated = false;
                        findViewById(R.id.fab).setVisibility(View.GONE);
                        findViewById(R.id.fab_associated).setVisibility(View.VISIBLE);
                        SharedPreferences.Editor editor = sharedPref.edit();
                        editor.putBoolean(SharedPrefConst.ASSOCIATED_STATUS, mAssociated);
                        editor.commit();
                        Toast.makeText(PushActivity.this, "disassociation failure", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_SUCCESS.equals(action)) {
                Log.v(TAG, "disassociate success");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mDisassociateMenuItem != null) {
                            mDisassociateMenuItem.setVisible(false);
                        }
                        mAssociated = false;
                        findViewById(R.id.fab).setVisibility(View.VISIBLE);
                        findViewById(R.id.fab_associated).setVisibility(View.GONE);
                        SharedPreferences.Editor editor = sharedPref.edit();
                        editor.putBoolean(SharedPrefConst.ASSOCIATED_STATUS, mAssociated);
                        editor.commit();
                        Toast.makeText(PushActivity.this, "disassociation success", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_SET_MESSAGE_SUCCESS.equals(action)) {
                Log.v(TAG, "set message success");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(PushActivity.this, "set message success", Toast.LENGTH_SHORT).show();
                    }
                });
            } else if (BluetoothEvents.BT_EVENT_DEVICE_MESSAGE_FAILURE.equals(action)) {
                Log.v(TAG, "set message failure");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(PushActivity.this, "set message failure", Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
    };

    @Override
    public void onResume() {
        super.onResume();
        mSingleton.setAssociate(true);
        mSingleton.setSingletonListener(this);
        mSingleton.bindService(getApplicationContext());
        dotProgressBar.setAlpha(0);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mSingleton.stopPushTask();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    }

    @Override
    public String getPassword() {
        return mPassword;
    }

    @Override
    public boolean getDebugMode() {
        return mDebugMode;
    }

    @Override
    public Context getContext() {
        return this;
    }

    @Override
    public void setPassword(String pass) {

        if (pass != null && !pass.equals("")) {
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.putString(SharedPrefConst.DEVICE_PASSWORD_FIELD, pass);
            editor.commit();
            mPassword = pass;
            mSingleton.setPassword(mPassword);
        }
    }

    @Override
    public void setDebugMode(boolean status) {
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putBoolean(SharedPrefConst.DEBUG_MODE_FIELD, status);
        editor.commit();
        mDebugMode = status;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                nvDrawer.getMenu().findItem(R.id.debug_mode_item).setTitle(getString(R.string.debug_mode_title) + " " + mDebugMode);
                if (mDebugMode) {
                    toolbar.getMenu().findItem(R.id.clear_btn).setVisible(true);
                    debugTv.setVisibility(View.VISIBLE);
                } else {
                    toolbar.getMenu().findItem(R.id.clear_btn).setVisible(false);
                    debugTv.setVisibility(View.GONE);
                }
            }
        });
    }

    @Override
    public void uploadPassword(String oldPass) {
        if (oldPass != null && !oldPass.equals("")) {
            mSingleton.uploadPassword(oldPass);
        }
    }

    @Override
    public void sendAssociationCode(String code) {
        mSingleton.sendAssociationCode(code);
    }

    @Override
    public void sendAssociationCodeFail() {
        mSingleton.sendAssociationCodeFail();
    }

    @Override
    public void generateNewAesKey() {
        mSingleton.generateNewAesKey();
    }

    @Override
    public void disassociate() {
        mSingleton.disassociate();
    }

    @Override
    public String getTopMessage() {
        return mSingleton.getTopMessage();
    }

    @Override
    public String getBotttomMessage() {
        return mSingleton.getBotttomMessage();
    }

    @Override
    public void setMessage(String topMessage, String bottomMessage) {
        mSingleton.setMessage(topMessage, bottomMessage);
    }

    private void clearReplaceDebugTv(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                debugTv.setText(text + System.getProperty("line.separator"));
            }
        });
    }

    @Override
    public void onBind() {
        if (mSingleton != null) {
            mSingleton.setServiceListener(mBtnListener);
        }
    }

    /**
     * add filter to intent to receive notification from bluetooth service
     *
     * @return intent filter
     */
    private static IntentFilter makeGattUpdateIntentFilter() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_USER_ACTION_REQUIRED);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_DISCONNECTED);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_ASSOCIATION_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_KEYS_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_PASSWORD_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_FAILURE);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_DISASSOCIATE_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_SET_MESSAGE_SUCCESS);
        intentFilter.addAction(BluetoothEvents.BT_EVENT_DEVICE_MESSAGE_FAILURE);
        return intentFilter;
    }
}
