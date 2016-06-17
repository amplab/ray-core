// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.input;

import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.input.InputClient;
import org.chromium.mojom.input.InputService;

import java.util.HashSet;
import java.util.Set;

/**
 * Android implementation of Input.
 */
public class InputServiceImpl implements InputService {
    /**
     * The currently connected clients.
     */
    private static final Set<InputClient> sClients = new HashSet<InputClient>();
    /**
     * The currently connected ready clients. A client is ready if the service is not waiting for a
     * callback.
     */
    private static final Set<InputClient> sReadyClients = new HashSet<InputClient>();

    /**
     * Dispatches the onBack event to all connected and ready clients. This needs to be called when
     * the user hits the back button. This returns <code>true</code> if the back button has been
     * dispatched to a client, and <code>false</code> otherwise.
     */
    public static boolean onBackButton() {
        if (sReadyClients.isEmpty()) {
            return false;
        }
        final Set<InputClient> readyClients = new HashSet<InputClient>(sReadyClients);
        sReadyClients.clear();
        for (final InputClient client : readyClients) {
            client.onBackButton(new InputClient.OnBackButtonResponse() {

                @Override
                public void call() {
                    if (sClients.contains(client)) {
                        sReadyClients.add(client);
                    }
                }
            });
        }
        return true;
    }

    /**
     * @see InputService#setClient(InputClient)
     */
    @Override
    public void setClient(final InputClient client) {
        if (client instanceof Interface.Proxy) {
            ((Interface.Proxy) client)
                    .getProxyHandler()
                    .setErrorHandler(new ConnectionErrorHandler() {
                        @Override
                        public void onConnectionError(MojoException e) {
                            sClients.remove(client);
                            sReadyClients.remove(client);
                        }
                    });
        }
        sClients.add(client);
        sReadyClients.add(client);
    }

    /**
     * @see InputService#close()
     */
    @Override
    public void close() {}

    /**
     * @see InputService#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {}
}
