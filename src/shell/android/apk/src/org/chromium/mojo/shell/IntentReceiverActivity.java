// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

/**
 * Activity that will forward received intents to the {@link IntentReceiverRegistry}.
 */
public class IntentReceiverActivity extends Activity {
    public static final String CATEGORY_START_ACTIVITY_FOR_RESULT = "C_startActivityForResult";
    public static final String EXTRA_START_ACTIVITY_FOR_RESULT_INTENT = "intent";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (processIntent(getIntent())) {
            if (isTaskRoot()) {
                finishAndRemoveTask();
            } else {
                finish();
            }
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        // We should always create a new activity instance, not reuse an existing
        // IntentReceiverActivity.
        assert false;
    }

    /**
     * Returns true if the intent has been completely processed, false otherwise (e.g.: we should
     * wait for an asynchronous result).
     */
    private boolean processIntent(Intent intent) {
        if (intent.getCategories() != null
                && intent.getCategories().contains(CATEGORY_START_ACTIVITY_FOR_RESULT)) {
            if (!intent.hasExtra(EXTRA_START_ACTIVITY_FOR_RESULT_INTENT)) {
                return true;
            }
            startActivityForResult(
                    intent.<Intent>getParcelableExtra(EXTRA_START_ACTIVITY_FOR_RESULT_INTENT),
                    Integer.parseInt(intent.getAction()));
            return false;
        }
        IntentReceiverRegistry.getInstance().onIntentReceived(intent);
        return true;
    }

    /**
     * @see Activity#onActivityResult(int, int, Intent)
     */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        IntentReceiverRegistry.getInstance().onActivityResult(requestCode, resultCode, data);
        if (isTaskRoot()) {
            finishAndRemoveTask();
        } else {
            finish();
        }
    }
}
