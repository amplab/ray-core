// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.os.HandlerThread;

import org.chromium.base.annotations.JNINamespace;

/**
 * Handler thread associated with a native message loop.
 */
@JNINamespace("shell")
public class NativeHandlerThread extends HandlerThread {
    // The native message loop.
    private long mNativeMessageLoop = 0;

    /**
     * Constructor.
     */
    public NativeHandlerThread(String name) {
        super(name);
    }

    /**
     * @see HandlerThread#run()
     */
    @Override
    public void run() {
        try {
            super.run();
        } finally {
            if (mNativeMessageLoop != 0) {
                nativeDeleteMessageLoop(mNativeMessageLoop);
            }
        }
    }

    /**
     * @see HandlerThread#onLooperPrepared()
     */
    @Override
    protected void onLooperPrepared() {
        if (mNativeMessageLoop == 0) {
            mNativeMessageLoop = nativeCreateMessageLoop();
        }
        super.onLooperPrepared();
    }

    native long nativeCreateMessageLoop();

    native void nativeDeleteMessageLoop(long messageLoop);
}
