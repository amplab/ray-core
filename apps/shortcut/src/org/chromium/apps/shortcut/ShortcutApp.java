// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.apps.shortcut;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.Pair;
import org.chromium.mojom.mojo.NetworkService;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.mojo.UrlLoader;
import org.chromium.mojom.mojo.UrlLoader.StartResponse;
import org.chromium.mojom.mojo.UrlRequest;
import org.chromium.mojom.mojo.UrlResponse;

/**
 * Mojo application that creates shortcuts on android.
 */
public class ShortcutApp implements ApplicationDelegate {
    private final Context mContext;
    private final Core mCore;

    /**
     * Constructor.
     */
    public ShortcutApp(Context context, Core core) {
        mContext = context;
        mCore = core;
    }

    /**
     * @see ApplicationDelegate#initialize(Shell, String[], String)
     */
    @Override
    public void initialize(Shell shell, String[] args, String url) {
        if (args.length < 3 || args.length > 4) {
            mCore.getCurrentRunLoop().quit();
            return;
        }
        final String shortcutName = args[1];
        final String shortcutUrl = args[2];
        if (args.length == 3) {
            createShortcut(shortcutName, shortcutUrl, null);
            return;
        }
        try (NetworkService network_service = ShellHelper.connectToService(
                     mCore, shell, "mojo:network_service", NetworkService.MANAGER)) {
            Pair<UrlLoader.Proxy, InterfaceRequest<UrlLoader>> url_loader_request =
                    UrlLoader.MANAGER.getInterfaceRequest(mCore);
            network_service.createUrlLoader(url_loader_request.second);
            final UrlLoader loader = url_loader_request.first;
            UrlRequest request = new UrlRequest();
            request.autoFollowRedirects = true;
            request.method = "GET";
            request.url = args[3];
            loader.start(request, new StartResponse() {
                @Override
                public void call(UrlResponse response) {
                    loader.close();
                    Bitmap bitmap =
                            BitmapFactory.decodeStream(new DataPipeInputStream(response.body));
                    if (bitmap == null) {
                        System.err.println(
                                "Unable to download icon. Creating shortcut with default icon.");
                    }
                    createShortcut(shortcutName, shortcutUrl, bitmap);
                }
            });
        }
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

    private void createShortcut(String shortcutName, String shortcutUrl, Bitmap bitmap) {
        Intent shortcutIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(shortcutUrl));

        Intent addIntent = new Intent();
        addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
        addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, shortcutName);
        if (bitmap != null) {
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON, bitmap);
        }

        addIntent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
        mContext.sendBroadcast(addIntent);

        mCore.getCurrentRunLoop().quit();
    }

    public static void mojoMain(
            Context context, Core core, MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(new ShortcutApp(context, core), core, applicationRequestHandle);
    }
}
