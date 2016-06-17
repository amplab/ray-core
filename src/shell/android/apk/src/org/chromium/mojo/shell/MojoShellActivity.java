// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.content.Intent;

/**
 * Entry point for the Mojo Shell application.
 */
public class MojoShellActivity extends BaseActivity {
    /**
     * @see BaseActivity#onCreateWithPermissions()
     */
    @Override
    protected void onCreateWithPermissions() {
        Intent serviceIntent = new Intent(this, ShellService.class);
        // Copy potential startup arguments.
        serviceIntent.putExtras(getIntent());
        if (getIntent().getData() != null) {
            serviceIntent.putExtra(
                    ShellService.APPLICATION_URL_EXTRA, getIntent().getData().toString());
        }
        startService(serviceIntent);
        finish();
    }
}
