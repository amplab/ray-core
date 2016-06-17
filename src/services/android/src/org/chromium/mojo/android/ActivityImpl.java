// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.android;

import android.app.Application.ActivityLifecycleCallbacks;
import android.content.ActivityNotFoundException;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.SoundEffectConstants;
import android.view.View;

import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.activity.Activity;
import org.chromium.mojom.activity.AuralFeedbackType;
import org.chromium.mojom.activity.ComponentName;
import org.chromium.mojom.activity.HapticFeedbackType;
import org.chromium.mojom.activity.Intent;
import org.chromium.mojom.activity.ScreenOrientation;
import org.chromium.mojom.activity.StringExtra;
import org.chromium.mojom.activity.SystemUiVisibility;
import org.chromium.mojom.activity.TaskDescription;
import org.chromium.mojom.activity.UserFeedback;

/**
 * Android implementation of Activity.
 *
 * This is a port of
 * https://github.com/flutter/engine/blob/master/sky/services/activity/src/org/domokit/activity/ActivityImpl.java
 */
public class ActivityImpl implements Activity {
    private static final String TAG = "AndroidImpl";

    private final android.app.Activity mActivity;
    private int mVisibility = SystemUiVisibility.STANDARD;

    private static class UserFeedBackImpl implements UserFeedback {
        private final View mView;

        public UserFeedBackImpl(View view) {
            mView = view;
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}

        /**
         * @see UserFeedback#performHapticFeedback(int)
         */
        @Override
        public void performHapticFeedback(int type) {
            int androidType = 0;
            switch (type) {
                case HapticFeedbackType.LONG_PRESS:
                    androidType = HapticFeedbackConstants.LONG_PRESS;
                    break;
                case HapticFeedbackType.VIRTUAL_KEY:
                    androidType = HapticFeedbackConstants.VIRTUAL_KEY;
                    break;
                case HapticFeedbackType.KEYBOARD_TAP:
                    androidType = HapticFeedbackConstants.KEYBOARD_TAP;
                    break;
                case HapticFeedbackType.CLOCK_TICK:
                    androidType = HapticFeedbackConstants.CLOCK_TICK;
                    break;
                default:
                    Log.e(TAG, "Unknown HapticFeedbackType " + type);
                    return;
            }
            mView.performHapticFeedback(
                    androidType, HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);
        }

        /**
         * @see UserFeedback#performAuralFeedback(int)
         */
        @Override
        public void performAuralFeedback(int type) {
            int androidType = 0;
            switch (type) {
                case AuralFeedbackType.CLICK:
                    androidType = SoundEffectConstants.CLICK;
                    break;
                case AuralFeedbackType.NAVIGATION_LEFT:
                    androidType = SoundEffectConstants.NAVIGATION_LEFT;
                    break;
                case AuralFeedbackType.NAVIGATION_UP:
                    androidType = SoundEffectConstants.NAVIGATION_UP;
                    break;
                case AuralFeedbackType.NAVIGATION_RIGHT:
                    androidType = SoundEffectConstants.NAVIGATION_RIGHT;
                    break;
                case AuralFeedbackType.NAVIGATION_DOWN:
                    androidType = SoundEffectConstants.NAVIGATION_DOWN;
                    break;
                default:
                    Log.e(TAG, "Unknown AuralFeedbackType " + type);
                    return;
            }
            mView.playSoundEffect(androidType);
        }
    }

    /**
     * Constructor.
     */
    public ActivityImpl(android.app.Activity activity) {
        mActivity = activity;
        activity.getApplication().registerActivityLifecycleCallbacks(
                new ActivityLifecycleCallbacks() {

                    @Override
                    public void onActivityStopped(android.app.Activity activity) {}

                    @Override
                    public void onActivityStarted(android.app.Activity activity) {}

                    @Override
                    public void onActivitySaveInstanceState(
                            android.app.Activity activity, Bundle bundle) {}

                    @Override
                    public void onActivityResumed(android.app.Activity activity) {
                        if (activity == mActivity) {
                            updateSystemUiVisibility();
                        }
                    }

                    @Override
                    public void onActivityPaused(android.app.Activity activity) {}

                    @Override
                    public void onActivityDestroyed(android.app.Activity activity) {}

                    @Override
                    public void onActivityCreated(android.app.Activity activity, Bundle bundle) {}
                });
    }

    @Override
    public void close() {}

    @Override
    public void onConnectionError(MojoException e) {}

    /**
     * @see Activity#getUserFeedback(InterfaceRequest)
     */
    @Override
    public void getUserFeedback(final InterfaceRequest<UserFeedback> userFeedback) {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                UserFeedback.MANAGER.bind(
                        new UserFeedBackImpl(mActivity.getWindow().getDecorView()), userFeedback);
            }
        });
    }

    /**
     * @see Activity#startActivity(Intent)
     */
    @Override
    public void startActivity(Intent intent) {
        final Uri uri = Uri.parse(intent.url);
        final android.content.Intent androidIntent = new android.content.Intent(intent.action, uri);

        if (intent.component != null) {
            ComponentName component = intent.component;
            android.content.ComponentName androidComponent =
                    new android.content.ComponentName(component.packageName, component.className);
            androidIntent.setComponent(androidComponent);
        }

        if (intent.stringExtras != null) {
            for (StringExtra extra : intent.stringExtras) {
                androidIntent.putExtra(extra.name, extra.value);
            }
        }

        if (intent.flags != 0) {
            androidIntent.setFlags(intent.flags);
        }

        if (intent.type != null) {
            // Intent.setType() clears data URI; that's why we call setDataAndType() here.
            androidIntent.setDataAndType(uri, intent.type);
        }

        try {
            mActivity.startActivity(androidIntent);
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "Unable to startActivity", e);
        }
    }

    /**
     * @see Activity#finishCurrentActivity()
     */
    @Override
    public void finishCurrentActivity() {
        Log.e(TAG, "finishCurrentActivity() is not implemented.");
    }

    /**
     * @see Activity#setTaskDescription(TaskDescription)
     */
    @Override
    public void setTaskDescription(TaskDescription description) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            return;
        }
        mActivity.setTaskDescription(new android.app.ActivityManager.TaskDescription(
                description.label, null, description.primaryColor));
    }

    /**
     * @see Activity#setSystemUiVisibility(int)
     */
    @Override
    public void setSystemUiVisibility(int visibility) {
        mVisibility = visibility;
        updateSystemUiVisibility();
    }

    /**
     * @see Activity#setRequestedOrientation(int)
     */
    @Override
    public void setRequestedOrientation(int orientation) {
        int androidOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;

        switch (orientation) {
            case ScreenOrientation.UNSPECIFIED:
                androidOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
                break;
            case ScreenOrientation.LANDSCAPE:
                androidOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
                break;
            case ScreenOrientation.PORTRAIT:
                androidOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
                break;
            case ScreenOrientation.NOSENSOR:
                androidOrientation = ActivityInfo.SCREEN_ORIENTATION_NOSENSOR;
                break;
            default:
                Log.w(TAG,
                        "Unable to set the requested orientation. Unknown value: " + orientation);
                break;
        }

        mActivity.setRequestedOrientation(androidOrientation);
    }

    private void updateSystemUiVisibility() {
        int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;

        if (mVisibility >= SystemUiVisibility.FULLSCREEN) {
            flags |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN;
        }

        if (mVisibility >= SystemUiVisibility.IMMERSIVE) {
            flags |= View.SYSTEM_UI_FLAG_IMMERSIVE | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        }

        final int finalFlags = flags;
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.getWindow().getDecorView().setSystemUiVisibility(finalFlags);
            }
        });
    }
}
