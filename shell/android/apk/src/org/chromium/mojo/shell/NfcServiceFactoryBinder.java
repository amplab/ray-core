// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.nfc.NfcAdapter;

import org.chromium.base.ApplicationStatus;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojom.nfc.Nfc;

/**
 * A ServiceFactoryBinder for the nfc service.
 */
final class NfcServiceFactoryBinder implements ServiceFactoryBinder<Nfc> {
    private final NfcImpl mNfcImpl;

    NfcServiceFactoryBinder(String requestorUrl) {
        NfcAdapter nfcAdapter =
                NfcAdapter.getDefaultAdapter(ApplicationStatus.getApplicationContext());
        if (nfcAdapter == null || !nfcAdapter.isNdefPushEnabled()) {
            android.util.Log.w("Nfc", "NFC is not available");
            mNfcImpl = null;
        } else {
            String packageName = ApplicationStatus.getApplicationContext().getPackageName();
            mNfcImpl = new NfcImpl(nfcAdapter, packageName, requestorUrl);
        }
    }

    @Override
    public void bind(InterfaceRequest<Nfc> request) {
        if (mNfcImpl == null) {
            request.close();
        } else {
            Nfc.MANAGER.bind(mNfcImpl, request);
        }
    }

    @Override
    public String getInterfaceName() {
        return Nfc.MANAGER.getName();
    }
}
