// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.sharing;

import android.content.Context;
import android.content.Intent;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.SharingService;
import org.chromium.mojom.mojo.Shell;

/**
 * Android Mojo application which implements |SharingService| interface.
 */
public class SharingApp implements ApplicationDelegate, SharingService {
    private final Context mContext;

    public SharingApp(Context context) {
        mContext = context;
    }

    /**
     * Share text using an Android Intent.
     *
     * @see org.chromium.mojom.mojo.SharingService#shareText(String)
     */
    @Override
    public void shareText(String text) {
        Intent intent = new Intent(Intent.ACTION_SEND)
                                .putExtra(Intent.EXTRA_TEXT, text)
                                .setType("text/plain")
                                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
    }

    /**
     * When connection is closed, we stop receiving location updates.
     *
     * @see org.chromium.mojo.bindings.Interface#close()
     */
    @Override
    public void close() {}

    /**
     * @see org.chromium.mojo.bindings.Interface#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {}

    /**
     * @see ApplicationDelegate#initialize(Shell, String[], String)
     */
    @Override
    public void initialize(Shell shell, String[] args, String url) {}

    /**
     * @see ApplicationDelegate#configureIncomingConnection(ApplicationConnection)
     */
    @Override
    public boolean configureIncomingConnection(ApplicationConnection connection) {
        connection.addService(new ServiceFactoryBinder<SharingService>() {
            @Override
            public void bind(InterfaceRequest<SharingService> request) {
                SharingService.MANAGER.bind(SharingApp.this, request);
            }

            @Override
            public String getInterfaceName() {
                return SharingService.MANAGER.getName();
            }
        });
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}

    public static void mojoMain(
            Context context, Core core, MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(new SharingApp(context), core, applicationRequestHandle);
    }
}
