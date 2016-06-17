// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.text.TextUtils;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.nfc.NfcData;
import org.chromium.mojom.nfc.NfcMessageSink;
import org.chromium.mojom.nfc.NfcReceiver;

import java.util.ArrayList;

/**
 * An ApplicationDelegate for the nfc service.
 */
final class NfcApplicationDelegate implements ApplicationDelegate {
    private Shell mShell;

    /**
     * @see ApplicationDelegate#initialize(Shell, String[], String)
     */
    @Override
    public void initialize(Shell shell, String[] args, String url) {
        mShell = shell;
    }

    /**
     * @see ApplicationDelegate#configureIncomingConnection(ApplicationConnection)
     */
    @Override
    public boolean configureIncomingConnection(ApplicationConnection connection) {
        connection.addService(new NfcServiceFactoryBinder(connection.getRequestorUrl()));
        if (TextUtils.isEmpty(connection.getRequestorUrl())) {
            connection.addService(new NfcMessageSinkServiceFactoryBinder());
        }
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}

    class NfcMessageSinkServiceFactoryBinder implements ServiceFactoryBinder<NfcMessageSink> {
        @Override
        public void bind(InterfaceRequest<NfcMessageSink> request) {
            NfcMessageSink.MANAGER.bind(new NfcMessageSinkImpl(), request);
        }

        @Override
        public String getInterfaceName() {
            return NfcMessageSink.MANAGER.getName();
        }
    }

    class NfcMessageSinkImpl implements NfcMessageSink {
        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}

        @Override
        public void onNfcMessage(byte[] data) {
            ArrayList<NfcReceiver> nfcReceivers = new ArrayList<NfcReceiver>();

            String[] registeredAppUrls = NfcDbManager.getInstance().getRegisteredApplications();

            if (registeredAppUrls != null) {
                for (String registeredAppUrl : registeredAppUrls) {
                    // Connect to registered app, add exposed NfcReciever to nfcReceivers list.
                    NfcReceiver nfcReceiver = ShellHelper.connectToService(
                            CoreImpl.getInstance(), mShell, registeredAppUrl, NfcReceiver.MANAGER);
                    if (nfcReceiver != null) {
                        nfcReceivers.add(nfcReceiver);
                    }
                }
            }

            NfcData nfcData = new NfcData();
            nfcData.data = data;
            for (final NfcReceiver nfcReceiver : nfcReceivers) {
                nfcReceiver.onReceivedNfcData(nfcData);
                nfcReceiver.close();
            }
        }
    }
}
