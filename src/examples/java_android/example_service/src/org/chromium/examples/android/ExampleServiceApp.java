// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.examples.android;

import android.content.Context;
import android.util.Log;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.ExampleService;
import org.chromium.mojom.mojo.Shell;

class ExampleServiceApp implements ApplicationDelegate {
    private static final String TAG = "ExampleServiceApp";

    public static class ExampleServiceImpl implements ExampleService {
        public ExampleServiceImpl() {}

        @Override
        public void ping(short pingValue, ExampleService.PingResponse callback) {
            callback.call(pingValue);
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}
    }

    @Override
    public void initialize(Shell shell, String[] args, String url) {
        Log.i(TAG, "ExampleServiceApp.Initialize() called.");
    }

    @Override
    public boolean configureIncomingConnection(ApplicationConnection connection) {
        Log.i(TAG, "ExampleServiceApp.ConfigureIncomingConnection() called.");
        connection.addService(new ServiceFactoryBinder<ExampleService>() {
            @Override
            public void bind(InterfaceRequest<ExampleService> request) {
                ExampleService.MANAGER.bind(new ExampleServiceImpl(), request);
            }

            @Override
            public String getInterfaceName() {
                return ExampleService.MANAGER.getName();
            }
        });
        return true;
    }

    @Override
    public void quit() {
        Log.i(TAG, "ExampleServiceApp.Quit() called.");
    }

    public static void mojoMain(@SuppressWarnings("unused") Context context, Core core,
            MessagePipeHandle applicationRequestHandle) {
        Log.i(TAG, "ExampleServiceApp.mojoMain() called.");
        ApplicationRunner.run(new ExampleServiceApp(), core, applicationRequestHandle);
    }
}
