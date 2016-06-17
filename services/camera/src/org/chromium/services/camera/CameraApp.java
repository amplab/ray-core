// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.camera;

import android.content.Context;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojom.mojo.CameraService;
import org.chromium.mojom.mojo.Shell;

class CameraApp implements ApplicationDelegate {
    private CameraServiceImpl mCameraServiceImpl;

    public CameraApp(Context context, Core core) {
        mCameraServiceImpl = new CameraServiceImpl(context, core);
    }

    @Override
    public void initialize(Shell shell, String[] args, String url) {
    }

    @Override
    public boolean configureIncomingConnection(final ApplicationConnection connection) {
        connection.addService(new ServiceFactoryBinder<CameraService>() {
            @Override
            public void bind(InterfaceRequest<CameraService> request) {
                if (mCameraServiceImpl.cameraInUse()) {
                    /* another application is using the camera */
                    /* TODO: support multiplexing the camera stream to multiple applications */
                    request.close();
                    return;
                }
                mCameraServiceImpl.openCamera();
                CameraService.MANAGER.bind(mCameraServiceImpl, request);
            }

            @Override
            public String getInterfaceName() {
                return CameraService.MANAGER.getName();
            }
        });
        return true;
    }

    @Override
    public void quit() {
        mCameraServiceImpl.cleanup();
    }

    public static void mojoMain(Context context, Core core,
                                MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(new CameraApp(context, core), core, applicationRequestHandle);
    }
}
