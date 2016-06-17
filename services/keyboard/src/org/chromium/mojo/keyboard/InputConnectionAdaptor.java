// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.keyboard;

import android.text.Editable;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.CorrectionInfo;
import android.view.inputmethod.EditorInfo;

import org.chromium.mojom.keyboard.CompletionData;
import org.chromium.mojom.keyboard.CorrectionData;
import org.chromium.mojom.keyboard.KeyboardClient;
import org.chromium.mojom.keyboard.SubmitAction;

/**
 * An adaptor between InputConnection and KeyboardClient.
 */
public class InputConnectionAdaptor extends BaseInputConnection {
    private KeyboardClient mClient;
    private Editable mEditable;

    public InputConnectionAdaptor(View view, KeyboardClient client, Editable editable,
                                  EditorInfo outAttrs) {
        super(view, true);
        assert client != null;
        mClient = client;
        mEditable = editable;
        outAttrs.initialSelStart = -1;
        outAttrs.initialSelEnd = -1;
        outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE;
    }

    @Override
    public Editable getEditable() {
        return mEditable;
    }

    @Override
    public boolean commitCompletion(CompletionInfo completion) {
        // TODO(abarth): Copy the data from |completion| to CompletionData.
        mClient.commitCompletion(new CompletionData());
        return super.commitCompletion(completion);
    }

    @Override
    public boolean commitCorrection(CorrectionInfo correction) {
        // TODO(abarth): Copy the data from |correction| to CompletionData.
        mClient.commitCorrection(new CorrectionData());
        return super.commitCorrection(correction);
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        mClient.commitText(text.toString(), newCursorPosition);
        return super.commitText(text, newCursorPosition);
    }

    @Override
    public boolean deleteSurroundingText(int beforeLength, int afterLength) {
        mClient.deleteSurroundingText(beforeLength, afterLength);
        return super.deleteSurroundingText(beforeLength, afterLength);
    }

    @Override
    public boolean setComposingRegion(int start, int end) {
        mClient.setComposingRegion(start, end);
        return super.setComposingRegion(start, end);
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {
        mClient.setComposingText(text.toString(), newCursorPosition);
        return super.setComposingText(text, newCursorPosition);
    }

    @Override
    public boolean setSelection(int start, int end) {
        mClient.setSelection(start, end);
        return super.setSelection(start, end);
    }

    // Number keys come through as key events instead of commitText!?
    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_UP) {
            // 1 appears to always be the value for newCursorPosition?
            mClient.commitText(String.valueOf(event.getNumber()), 1);
        }
        return super.sendKeyEvent(event);
    }

    @Override
    public boolean performEditorAction(int actionCode) {
        mClient.submit(SubmitAction.DONE);
        return true;
    }
}
