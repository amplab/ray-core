// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.authentication;

import android.content.Context;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.bindings.Connector;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.Pair;
import org.chromium.mojom.mojo.AuthenticatingUrlLoaderInterceptorMetaFactory;
import org.chromium.mojom.mojo.NetworkService;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.mojo.UrlLoaderInterceptorFactory;

/**
 * Service factory for the network service proxy that requests the real network service, configures
 * it for authentication and then forwards the calls from the client to the real network service.
 */
public class NetworkServiceProxyFactory implements ServiceFactoryBinder<NetworkService> {
    private final ApplicationConnection mConnection;
    private final Context mContext;
    private final Core mCore;
    private final Shell mShell;

    public NetworkServiceProxyFactory(
            ApplicationConnection connection, Context context, Core core, Shell shell) {
        mConnection = connection;
        mContext = context;
        mCore = core;
        mShell = shell;
    }

    /**
     * @see ServiceFactoryBinder#bind(InterfaceRequest)
     */
    @Override
    public void bind(InterfaceRequest<NetworkService> request) {
        NetworkService.Proxy network_service = ShellHelper.connectToService(
                mCore, mShell, "mojo:network_service", NetworkService.MANAGER);
        AuthenticationService authenticationService = new AuthenticationServiceImpl(
                mContext, mCore, mConnection.getRequestorUrl(), mShell);
        AuthenticatingUrlLoaderInterceptorMetaFactory metaFactory = ShellHelper.connectToService(
                mCore, mShell, "mojo:authenticating_url_loader_interceptor",
                AuthenticatingUrlLoaderInterceptorMetaFactory.MANAGER);
        Pair<UrlLoaderInterceptorFactory.Proxy, InterfaceRequest<UrlLoaderInterceptorFactory>>
                interceptor_factory_request =
                        UrlLoaderInterceptorFactory.MANAGER.getInterfaceRequest(mCore);
        metaFactory.createUrlLoaderInterceptorFactory(
                interceptor_factory_request.second, authenticationService);
        network_service.registerUrlLoaderInterceptor(interceptor_factory_request.first);
        forwardHandles(network_service.getProxyHandler().passHandle(), request.passHandle());
    }

    /**
     * @see ServiceFactoryBinder#getInterfaceName()
     */
    @Override
    public String getInterfaceName() {
        return NetworkService.MANAGER.getName();
    }

    /**
     * Forwards messages from h1 to h2 and back.
     */
    private void forwardHandles(MessagePipeHandle h1, MessagePipeHandle h2) {
        Connector c1 = new Connector(h1);
        Connector c2 = new Connector(h2);
        c1.setIncomingMessageReceiver(c2);
        c2.setIncomingMessageReceiver(c1);
        c1.start();
        c2.start();
    }
}
