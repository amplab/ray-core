// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.os.Parcelable;

import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.nfc.NfcMessageSink;

import java.util.ArrayDeque;

/**
 * Activity for receiving nfc messages.
 */
public class NfcReceiverActivity
        extends BaseActivity implements ShellService.IShellBindingActivity {
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
            sendNfcMessages(pendingIntent);
        }
        unbindService(mShellServiceConnection);
    }

    @Override
    public void onShellUnbound() {
        mShellService = null;
    }

    private void sendNfcMessages(Intent intent) {
        NfcMessageSink nfcMessageSink = ShellHelper.connectToService(CoreImpl.getInstance(),
                mShellService.getShell(), "mojo:nfc", NfcMessageSink.MANAGER);

        Parcelable[] rawMsgs = intent.getParcelableArrayExtra(NfcAdapter.EXTRA_NDEF_MESSAGES);

        // Only one message sent during the beam.
        NdefMessage msg = (NdefMessage) rawMsgs[0];

        // Records 0 to size-2 contains the MIME type, record size-1 is the AAR.
        NdefRecord[] records = msg.getRecords();
        for (int i = 0; i < (records.length - 1); i++) {
            byte[] payload = records[i].getPayload();
            if (payload != null) {
                nfcMessageSink.onNfcMessage(payload);
            }
        }
        nfcMessageSink.close();
    }
}
