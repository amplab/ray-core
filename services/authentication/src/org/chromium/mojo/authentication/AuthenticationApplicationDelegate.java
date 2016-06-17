// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.authentication;

import android.content.Context;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojom.mojo.Shell;

/**
 * Android service application implementing the AuthenticationService interface.
 */
public class AuthenticationApplicationDelegate implements ApplicationDelegate {
    private final Context mContext;
    private final Core mCore;
    private Shell mShell;

    public AuthenticationApplicationDelegate(Context context, Core core) {
        mContext = context;
        mCore = core;
    }

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
    public boolean configureIncomingConnection(final ApplicationConnection connection) {
        connection.addService(new ServiceFactoryBinder<AuthenticationService>() {

            @Override
            public void bind(InterfaceRequest<AuthenticationService> request) {
                AuthenticationService.MANAGER.bind(new AuthenticationServiceImpl(mContext, mCore,
                                                           connection.getRequestorUrl(), mShell),
                        request);
            }

            @Override
            public String getInterfaceName() {
                return AuthenticationService.MANAGER.getName();
            }
        });
        connection.addService(new NetworkServiceProxyFactory(connection, mContext, mCore, mShell));
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}
}
