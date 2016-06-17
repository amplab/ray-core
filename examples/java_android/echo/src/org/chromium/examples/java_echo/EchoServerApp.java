// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.examples.java_echo;

import android.content.Context;
import android.util.Log;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.examples.echo.Echo;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.Shell;

/**
 * This is an example Java Mojo Application that receives a message from an echo client and then
 * responds with that same message, prefixed by the string "Java EchoServer: ". The user does not
 * directly request that this application be loaded. Instead the Mojo shell will load this
 * application when the echo client requests that an echo server be loaded. See the README.md file
 * for usage.
 */
class EchoServerApp implements ApplicationDelegate {
    private static final String TAG = "JavaEchoServer";

    public static class EchoImpl implements Echo {
        @Override
        public void echoString(String value, EchoStringResponse callback) {
            Log.i(TAG, "Received: " + value);
            callback.call("Java EchoServer: " + value);
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}
    }

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
        connection.addService(new ServiceFactoryBinder<Echo>() {

            @Override
            public void bind(InterfaceRequest<Echo> request) {
                Echo.MANAGER.bind(new EchoImpl(), request);
            }

            @Override
            public String getInterfaceName() {
                return Echo.MANAGER.getName();
            }
        });
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}

    public static void mojoMain(@SuppressWarnings("unused") Context context, Core core,
            MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(new EchoServerApp(), core, applicationRequestHandle);
    }
}
