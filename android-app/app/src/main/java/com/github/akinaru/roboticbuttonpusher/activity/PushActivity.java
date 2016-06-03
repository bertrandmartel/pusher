/*
 * This file is part of Bluetooth LE Analyzer.
 * <p/>
 * Copyright (C) 2016  Bertrand Martel
 * <p/>
 * Foobar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * <p/>
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
package com.github.akinaru.roboticbuttonpusher.activity;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
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
import com.github.akinaru.roboticbuttonpusher.constant.SharedPrefConst;
import com.github.akinaru.roboticbuttonpusher.inter.IPushBtnListener;
import com.github.akinaru.roboticbuttonpusher.inter.ISingletonListener;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherError;
import com.github.akinaru.roboticbuttonpusher.model.ButtonPusherState;
import com.github.silvestrpredko.dotprogressbar.DotProgressBar;


/**
 * Analyzer activity
 *
 * @author Bertrand Martel
 */
public class PushActivity extends BaseActivity implements ISingletonListener {

    private String TAG = this.getClass().getName();

    private static final boolean DEFAULT_DEBUG_MODE = false;

    private String mDeviceName;

    private String mPassword;

    private boolean mDebugMode;

    private FloatingActionButton button;

    private PushSingleton mSingleton;

    /**
     * shared preference object.
     */
    private SharedPreferences sharedPref;

    private static final int REQUEST_PERMISSION_COARSE_LOCATION = 2;

    protected void onCreate(Bundle savedInstanceState) {

        setLayout(R.layout.activity_button_push);
        super.onCreate(savedInstanceState);

        Log.v(TAG, "oncreate");

        mAnimationScaleUp = AnimationUtils.loadAnimation(this, R.anim.scale_up);
        mAnimationScaleDown = AnimationUtils.loadAnimation(this, R.anim.scale_down);
        mAnimationDefaultScaleUp = AnimationUtils.loadAnimation(this, R.anim.scale_default_up);

        dotProgressBar = (DotProgressBar) findViewById(R.id.dot_progress_bar);
        mFailureButton = (FloatingActionButton) findViewById(R.id.failure_button);

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
                        button.setVisibility(View.VISIBLE);
                        button.startAnimation(mAnimationDefaultScaleUp);
                    }
                });

            }
        });

        //shared preference
        sharedPref = getSharedPreferences(SharedPrefConst.PREFERENCES, Context.MODE_PRIVATE);

        mDeviceName = sharedPref.getString(SharedPrefConst.DEVICE_NAME_FIELD, SharedPrefConst.DEFAULT_DEVICE_NAME);
        mPassword = sharedPref.getString(SharedPrefConst.DEVICE_PASSWORD_FIELD, SharedPrefConst.DEFAULT_PASSWORD);
        mDebugMode = sharedPref.getBoolean(SharedPrefConst.DEBUG_MODE_FIELD, DEFAULT_DEBUG_MODE);

        mImgSelection = (FloatingActionButton) findViewById(R.id.img_selection);

        mSingleton = PushSingleton.getInstance();
        mSingleton.setSingletonListener(this);

        button = (FloatingActionButton) findViewById(R.id.fab);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

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
            }
        });

        if (Build.VERSION.SDK_INT >= 23) {
            if (checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                Log.i(TAG, "requesting location permission");
                requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_PERMISSION_COARSE_LOCATION);
            }
        }

        Log.i(TAG, "ok");
        mSingleton.bindService(getApplicationContext());
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_PERMISSION_COARSE_LOCATION: {
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {

                } else {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(PushActivity.this, "permission coarse location required for ble scan", Toast.LENGTH_SHORT).show();
                        }
                    });
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
        nvDrawer.getMenu().findItem(R.id.devicename_item).setTitle(getString(R.string.menu_device_name) + " " + mDeviceName);

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
        mSingleton.stopPushTask();
        mSingleton.unbindService(getApplicationContext());
        Log.v(TAG, "onDestroy");
    }

    @Override
    public void onResume() {
        super.onResume();
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
    public String getDeviceName() {
        return mDeviceName;
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
    public void setDeviceName(String deviceName) {

        if (deviceName != null && !deviceName.equals("")) {
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.putString(SharedPrefConst.DEVICE_NAME_FIELD, deviceName);
            editor.commit();
            mDeviceName = deviceName;
            mSingleton.setDeviceName(mDeviceName);
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    nvDrawer.getMenu().findItem(R.id.devicename_item).setTitle(getString(R.string.menu_device_name) + " " + mDeviceName);
                }
            });
        }
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
}
