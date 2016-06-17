// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.content.Context;
import android.content.Intent;

import org.chromium.base.ApplicationStatus;
import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.native_viewport.NativeViewportSupportService;
import org.chromium.mojom.native_viewport.NativeViewportSupportService.CreateNewNativeWindowResponse;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

final class NativeViewportSupportApplicationDelegate implements ApplicationDelegate {
    /**
     * Map of active UI tasks (activities), keyed by their UUID.
     */
    private static Map<String, CreateNewNativeWindowResponse> sActiveTasks =
            new HashMap<String, CreateNewNativeWindowResponse>();

    private class NativeViewportSupportServiceFactoryBinder
            implements ServiceFactoryBinder<NativeViewportSupportService> {
        @Override
        public void bind(InterfaceRequest<NativeViewportSupportService> request) {
            NativeViewportSupportService.MANAGER.bind(
                    new NativeViewportSupportServiceImpl(), request);
        }

        @Override
        public String getInterfaceName() {
            return NativeViewportSupportService.MANAGER.getName();
        }
    }

    private class NativeViewportSupportServiceImpl implements NativeViewportSupportService {
        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}

        @Override
        public void createNewNativeWindow(CreateNewNativeWindowResponse callback) {
            String viewportId = UUID.randomUUID().toString();

            sActiveTasks.put(viewportId, callback);

            Context context = ApplicationStatus.getApplicationContext();
            Intent newDocumentIntent = new Intent(context, ViewportActivity.class);
            newDocumentIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
            newDocumentIntent.addFlags(Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            newDocumentIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            newDocumentIntent.putExtra("ViewportId", viewportId);

            context.startActivity(newDocumentIntent);
        }
    }

    public static void viewportClosed(String viewportId) {
        assert sActiveTasks.containsKey(viewportId);
        sActiveTasks.get(viewportId).call();
        sActiveTasks.remove(viewportId);
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
        if (!connection.getRequestorUrl().equals("mojo://native_viewport_service/")) {
            return false;
        }
        connection.addService(new NativeViewportSupportServiceFactoryBinder());
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}
}
