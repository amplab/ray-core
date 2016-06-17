// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

import java.util.ArrayList;
import java.util.List;

/**
 * Base activity for any mojo shell activity that will start the shell service. This activity will
 * ensure that the permission for the shell service are requested and obtained before starting the
 * shell.
 */
public abstract class BaseActivity extends Activity {
    private static final String[] PERMISSIONS = {
            Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.CAMERA,
            Manifest.permission.INSTALL_SHORTCUT, Manifest.permission.INTERNET,
            Manifest.permission.NFC, Manifest.permission.READ_CONTACTS,
            Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.RECORD_AUDIO,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
    };

    /**
     * @see Activity#onCreate(Bundle)
     */
    @Override
    protected final void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        List<String> requested_permissions = new ArrayList<String>();
        for (String permission : PERMISSIONS) {
            if (ContextCompat.checkSelfPermission(this, permission)
                    != PackageManager.PERMISSION_GRANTED) {
                requested_permissions.add(permission);
            }
        }
        if (requested_permissions.size() > 0) {
            ActivityCompat.requestPermissions(this,
                    requested_permissions.toArray(new String[requested_permissions.size()]), 0);
            return;
        }

        onCreateWithPermissions();
    }

    /**
     * @see Activity#onRequestPermissionsResult(int, java.lang.String[], int[])
     */
    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        for (int result : grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                finishAndRemoveTask();
            }
        }
        onCreateWithPermissions();
    }

    /**
     * This method is called when the module ensured that all needed permissions have been requested
     * and accepted by the user.
     */
    abstract void onCreateWithPermissions();
}
