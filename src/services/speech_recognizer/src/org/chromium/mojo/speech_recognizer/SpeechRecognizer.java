// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.speech_recognizer;

import android.content.Context;

import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;

final class SpeechRecognizer {
    public static void mojoMain(
            Context context, Core core, MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(
                new SpeechRecognizerApplicationDelegate(context), core, applicationRequestHandle);
    }
}
