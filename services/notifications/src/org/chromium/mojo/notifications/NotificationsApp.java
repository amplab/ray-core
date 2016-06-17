// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.notifications;

import android.content.Context;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.notifications.NotificationService;

/**
 * Android service application implementing the NotificationService interface.
 */
public class NotificationsApp implements ApplicationDelegate {
    private final Context mContext;
    private final Core mCore;
    private Shell mShell;
    private int mNotificationIconResourceId;

    public NotificationsApp(Context context, Core core) {
        mContext = context;
        mCore = core;
    }

    /**
     * @see ApplicationDelegate#initialize(Shell, String[], String)
     */
    @Override
    public void initialize(Shell shell, String[] args, String url) {
        mShell = shell;
        if (args.length < 2) {
            android.util.Log.wtf("NotificationsApp", "mojo:notifications "
                            + "--args-for required to specify notification_icon_resource_id");
            return;
        }
        mNotificationIconResourceId = Integer.valueOf(args[1]);
    }

    /**
     * @see ApplicationDelegate#configureIncomingConnection(ApplicationConnection)
     */
    @Override
    public boolean configureIncomingConnection(final ApplicationConnection connection) {
        connection.addService(new ServiceFactoryBinder<NotificationService>() {

            @Override
            public void bind(InterfaceRequest<NotificationService> request) {
                NotificationService.MANAGER.bind(new NotificationServiceImpl(mContext, mCore,
                                                         mShell, mNotificationIconResourceId),
                        request);
            }

            @Override
            public String getInterfaceName() {
                return NotificationService.MANAGER.getName();
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
        ApplicationRunner.run(new NotificationsApp(context, core), core, applicationRequestHandle);
    }
}
