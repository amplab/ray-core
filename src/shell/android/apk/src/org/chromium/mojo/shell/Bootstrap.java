// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.content.Context;

import org.chromium.base.annotations.JNINamespace;

import java.io.File;

/**
 * Runnable used to bootstrap execution of Android Mojo application. For the JNI to work, we need a
 * Java class with the application classloader in the call stack. We load this class in the
 * application classloader and call into native from it to achieve that.
 */
@JNINamespace("shell")
public class Bootstrap implements Runnable {
    private final Context mContext;
    private final File mBootstrapNativeLibrary;
    private final File mApplicationNativeLibrary;
    private final int mHandle;
    private final long mTracingId;
    private final long mRunApplicationPtr;

    public Bootstrap(Context context, long tracingId, File bootstrapNativeLibrary,
            File applicationNativeLibrary, int handle, long runApplicationPtr) {
        mContext = context;
        mTracingId = tracingId;
        mBootstrapNativeLibrary = bootstrapNativeLibrary;
        mApplicationNativeLibrary = applicationNativeLibrary;
        mHandle = handle;
        mRunApplicationPtr = runApplicationPtr;
    }

    @Override
    public void run() {
        System.load(mBootstrapNativeLibrary.getAbsolutePath());
        System.load(mApplicationNativeLibrary.getAbsolutePath());
        nativeBootstrap(mContext, mTracingId, mApplicationNativeLibrary.getAbsolutePath(), mHandle,
                mRunApplicationPtr);
    }

    native void nativeBootstrap(
            Context context, long id, String libraryPath, int handle, long runApplicationPtr);
}
