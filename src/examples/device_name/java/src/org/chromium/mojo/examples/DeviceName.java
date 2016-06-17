// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.examples;

import android.content.Context;
import android.os.Build;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Example Java class providing functionality to the native side.
 */
@JNINamespace("mojo::examples")
public class DeviceName {
    @CalledByNative
    private static String getName() {
        return Build.MODEL;
    }

    @CalledByNative
    private static String getApplicationClassName(Context context) {
        return context.getApplicationInfo().className;
    }
}
