// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.speech_recognizer;

import android.content.Context;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojom.mojo.Shell;

/**
 * An ApplicationDelegate for the SpeechRecognizer service.
 */
public final class SpeechRecognizerApplicationDelegate implements ApplicationDelegate {
    private Context mContext;

    public SpeechRecognizerApplicationDelegate(Context context) {
        mContext = context;
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
        connection.addService(new ServiceFactoryBinder<SpeechRecognizerService>() {
            @Override
            public void bind(InterfaceRequest<SpeechRecognizerService> request) {
                SpeechRecognizerService.MANAGER.bind(
                        new SpeechRecognizerServiceImpl(mContext), request);
            }

            @Override
            public String getInterfaceName() {
                return SpeechRecognizerService.MANAGER.getName();
            }
        });
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {}
}
