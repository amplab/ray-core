// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.text.TextUtils;
import android.util.Log;

import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.sharing.SharingSink;

import java.util.ArrayDeque;

/**
 * Activity for sharing data with shell apps.
 */
public final class SharingActivity
        extends BaseActivity implements ShellService.IShellBindingActivity {
    private static final String TAG = "SharingActivity";

    private ArrayDeque<Intent> mPendingIntents = new ArrayDeque<Intent>();
    private ShellService mShellService;
    private ServiceConnection mShellServiceConnection;

    /**
     * @see BaseActivity#onCreateWithPermissions()
     */
    @Override
    void onCreateWithPermissions() {
        Intent serviceIntent = new Intent(this, ShellService.class);
        startService(serviceIntent);
        mShellServiceConnection = new ShellService.ShellServiceConnection(this);
        bindService(serviceIntent, mShellServiceConnection, Context.BIND_AUTO_CREATE);
        onNewIntent(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        mPendingIntents.add(intent);
    }

    @Override
    public void onShellBound(ShellService shellService) {
        mShellService = shellService;
        finish();
        while (!mPendingIntents.isEmpty()) {
            Intent pendingIntent = mPendingIntents.remove();
            shareFromIntent(pendingIntent);
        }
        unbindService(mShellServiceConnection);
    }

    @Override
    public void onShellUnbound() {
        mShellService = null;
    }

    private void shareFromIntent(Intent intent) {
        String action = intent.getAction();
        if (!Intent.ACTION_SEND.equals(action)) {
            Log.w(TAG, "Wrong action sent to this activity: " + action + ".");
            return;
        }

        String type = intent.getType();
        if (!"text/plain".equals(type)) {
            Log.w(TAG, "Wrong data type sent to this activity: " + type + ".");
            return;
        }

        String text = intent.getStringExtra(Intent.EXTRA_TEXT);
        if (TextUtils.isEmpty(text)) {
            Log.w(TAG, "No text available to share.");
            return;
        }

        SharingSink sink = ShellHelper.connectToService(CoreImpl.getInstance(),
                mShellService.getShell(), "mojo:sharing", SharingSink.MANAGER);
        sink.onTextShared(text);
    }
}
