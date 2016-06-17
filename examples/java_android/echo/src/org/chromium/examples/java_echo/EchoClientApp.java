// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.examples.java_echo;

import android.content.Context;
import android.util.Log;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.examples.echo.Echo;
import org.chromium.mojo.examples.echo.Echo.EchoStringResponse;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojom.mojo.Shell;

/**
 * This is an example Java Mojo Application that sends a message to an echo server and then
 * prints the response received from the echo server to the Android logcat output. See
 * the README.md file for usage.
 */
class EchoClientApp implements ApplicationDelegate {
    private static final String TAG = "JavaEchoClient";
    private static final String DEFAULT_MESSAGE = "Hello Java!";

    /**
     * An implementation of {@link EchoStringResponse} that prints the response
     * and then quits.
     */
    private class EchoCallback implements EchoStringResponse {
        @Override
        public void call(String response) {
            Log.i(TAG, "Response: " + response);
            mCore.getCurrentRunLoop().quit();
        }
    }

    private Core mCore;

    public EchoClientApp(Core core) {
        mCore = core;
    }

    /**
     * Joins the strings in {@code strings} from {@code from} to {@code to} into a space-separated
     * String and returns the joined string.
     */
    private String join(String[] strings, int from, int to) {
        assert (strings != null && from >= 0 && from <= to && to < strings.length);
        StringBuffer sb = new StringBuffer();
        for (int i = from; i <= to; i++) {
            if (i > from) {
                sb.append(' ');
            }
            sb.append(strings[i]);
        }
        return sb.toString();
    }

    /**
     * @see ApplicationDelegate#initialize(Shell, String[], String)
     */
    @Override
    public void initialize(Shell shell, String[] args, String url) {
        Echo echo = ShellHelper.connectToService(mCore, shell, "mojo:echo_server", Echo.MANAGER);
        String messageToSend = DEFAULT_MESSAGE;
        if (args.length > 1) {
            messageToSend = join(args, 1, args.length - 1);
        }

        echo.echoString(messageToSend, new EchoCallback());
    }

    /**
     * @see ApplicationDelegate#configureIncomingConnection(ApplicationConnection)
     */
    @Override
    public boolean configureIncomingConnection(ApplicationConnection connection) {
        return false;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}

    public static void mojoMain(@SuppressWarnings("unused") Context context, Core core,
            MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(new EchoClientApp(core), core, applicationRequestHandle);
    }
}
