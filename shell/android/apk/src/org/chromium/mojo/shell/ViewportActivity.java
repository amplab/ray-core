// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.WindowManager;

import org.chromium.mojo.PlatformViewportAndroid;
import org.chromium.mojo.input.InputServiceImpl;

/**
 * Activity for displaying on the screen from the NativeViewportService.
 */
public class ViewportActivity extends Activity {
    private static Activity sCurrentActivity = null;

    public static Activity getCurrent() {
        return sCurrentActivity;
    }

    /**
     * @see Activity#onKeyDown(int, KeyEvent)
     */
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
            if (InputServiceImpl.onBackButton()) {
                return true;
            }
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!PlatformViewportAndroid.newActivityStarted(this)) {
            // We have not attached to a viewport, so there's no point continuing.
            finishAndRemoveTask();
        }

        // TODO(tonyg): Watch activities go back to the home screen within a
        // couple of seconds of detaching from adb. So for demonstration purposes,
        // we just keep the screen on. Eventually we'll want a solution for
        // allowing the screen to sleep without quitting the shell.
        // TODO(etiennej): Verify the above is still true after the switch to a Service model.
        UiModeManager uiModeManager = (UiModeManager) getSystemService(UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_WATCH) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        new RuntimeException("This activity instance should only ever receive one intent.");
    }

    /**
     * @see Activity#onPause()
     */
    @Override
    protected void onPause() {
        if (sCurrentActivity == this) sCurrentActivity = null;
        super.onPause();
    }

    /**
     * @see Activity#onResume()
     */
    @Override
    protected void onResume() {
        super.onResume();
        sCurrentActivity = this;
    }
}
