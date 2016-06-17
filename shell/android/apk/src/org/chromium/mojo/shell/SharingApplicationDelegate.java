// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.text.TextUtils;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.sharing.SharingSink;

final class SharingApplicationDelegate implements ApplicationDelegate {

    @Override
    public void initialize(Shell shell, String[] args, String url) {
    }

    @Override
    public boolean configureIncomingConnection(ApplicationConnection connection) {
        if (TextUtils.isEmpty(connection.getRequestorUrl())) {
            // This is the shell connecting to the service.
            connection.addService(new SharingSinkServiceFactoryBinder());
        }
        return true;
    }

    @Override
    public void quit() {}

    private final class SharingSinkServiceFactoryBinder
            implements ServiceFactoryBinder<SharingSink> {
        @Override
        public void bind(InterfaceRequest<SharingSink> request) {
            SharingSink.MANAGER.bind(new SharingSinkImpl(), request);
        }

        @Override
        public String getInterfaceName() {
            return SharingSink.MANAGER.getName();
        }
    }

    private final class SharingSinkImpl implements SharingSink {
        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}

        @Override
        public void onTextShared(String data) {
            // TODO(pylaligand): do something slightly less dumb with the data.
            System.out.println("Request to share data: " + data);
        }
    }
}
