// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.contacts;

import android.content.Context;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojom.contacts.ContactsService;
import org.chromium.mojom.mojo.Shell;

/**
 * Android service application implementing the ContactsService interface using Google play services
 * API.
 */
public class ContactsApplicationDelegate implements ApplicationDelegate {
    private final Context mContext;
    private final Core mCore;

    public ContactsApplicationDelegate(Context context, Core core) {
        mContext = context;
        mCore = core;
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
        connection.addService(new ServiceFactoryBinder<ContactsService>() {
            @Override
            public void bind(InterfaceRequest<ContactsService> request) {
                ContactsService.MANAGER.bind(new ContactsServiceImpl(mContext, mCore), request);
            }

            @Override
            public String getInterfaceName() {
                return ContactsService.MANAGER.getName();
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
        ApplicationRunner.run(
                new ContactsApplicationDelegate(context, core), core, applicationRequestHandle);
    }
}
