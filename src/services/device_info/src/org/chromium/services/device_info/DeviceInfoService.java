// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.device_info;

import android.content.Context;
import android.util.DisplayMetrics;
import android.view.WindowManager;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.DeviceInfo;
import org.chromium.mojom.mojo.Shell;

/**
 * Android Mojo application which implements |DeviceInfo| interface.
 */
class DeviceInfoService implements ApplicationDelegate, DeviceInfo {
    private final Context mContext;

    public DeviceInfoService(Context context) {
        mContext = context;
    }

    /**
     * Infer the device type from the screen's size.
     *
     * @see org.chromium.mojom.mojo.DeviceInfo#getDeviceType(DeviceInfo.GetDeviceTypeResponse)
     */
    @Override
    public void getDeviceType(DeviceInfo.GetDeviceTypeResponse callback) {
        DisplayMetrics metrics = getDisplayMetrics();
        if (metrics == null) {
            callback.call(DeviceInfo.DeviceType.HEADLESS);
            return;
        }

        double screen = getScreenSizeInInches(metrics);
        if (screen < 3.5) {
            callback.call(DeviceInfo.DeviceType.WATCH);
        } else if (screen < 6.5) {
            callback.call(DeviceInfo.DeviceType.PHONE);
        } else if (screen < 10.5) {
            callback.call(DeviceInfo.DeviceType.TABLET);
        } else {
            callback.call(DeviceInfo.DeviceType.TV);
        }
    }

    /**
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
        connection.addService(new ServiceFactoryBinder<DeviceInfo>() {
            @Override
            public void bind(InterfaceRequest<DeviceInfo> request) {
                DeviceInfo.MANAGER.bind(DeviceInfoService.this, request);
            }

            @Override
            public String getInterfaceName() {
                return DeviceInfo.MANAGER.getName();
            }
        });
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}

    private DisplayMetrics getDisplayMetrics() {
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        if (wm == null) {
            return null;
        }
        wm.getDefaultDisplay().getMetrics(metrics);
        return metrics;
    }

    private double getScreenSizeInInches(DisplayMetrics metrics) {
        double width = metrics.widthPixels;
        double height = metrics.heightPixels;
        double dpi = metrics.densityDpi;
        double wi = width / dpi;
        double hi = height / dpi;
        return Math.sqrt(Math.pow(wi, 2) + Math.pow(hi, 2));
    }

    public static void mojoMain(
            Context context, Core core, MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(new DeviceInfoService(context), core, applicationRequestHandle);
    }
}
