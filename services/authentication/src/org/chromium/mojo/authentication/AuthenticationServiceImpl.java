// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.authentication;

import android.accounts.AccountManager;
import android.content.Context;
import android.content.Intent;
import android.os.Parcel;

import com.google.android.gms.auth.GoogleAuthException;
import com.google.android.gms.auth.GoogleAuthUtil;
import com.google.android.gms.auth.UserRecoverableAuthException;
import com.google.android.gms.common.AccountPicker;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GooglePlayServicesUtil;

import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.bindings.DeserializationException;
import org.chromium.mojo.bindings.Message;
import org.chromium.mojo.bindings.SideEffectFreeCloseable;
import org.chromium.mojo.intent.IntentReceiver;
import org.chromium.mojo.intent.IntentReceiverManager;
import org.chromium.mojo.intent.IntentReceiverManager.RegisterActivityResultReceiverResponse;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.Handle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.Shell;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import java.util.ArrayList;
import java.util.HashMap;

/**
 * Implementation of AuthenticationService from services/authentication/authentication.mojom
 */
public class AuthenticationServiceImpl
        extends SideEffectFreeCloseable implements AuthenticationService {
    // The current version of the database. This must be incremented each time Db definition in
    // authentication_impl_db.mojom is changed in a non backward-compatible way.
    private static final int VERSION = 0;

    // Type of google accounts.
    private static final String GOOGLE_ACCOUNT_TYPE = "com.google";

    // Type of the accounts that this service allows the user to pick.
    private static final String[] ACCOUNT_TYPES = new String[] {GOOGLE_ACCOUNT_TYPE};

    // Name of the extra key used to store the intent we actually want to send. This value must
    // match IntentReceiverActivity.EXTRA_START_ACTIVITY_FOR_RESULT_INTENT.
    private static final String EXTRA_START_ACTIVITY_FOR_RESULT_INTENT = "intent";

    /**
     * An callback that takes a serialized intent, add the intent the shell needs to send and start
     * the container intent.
     */
    private final class RegisterActivityResultReceiverCallback
            implements RegisterActivityResultReceiverResponse {
        /**
         * The intent that the requesting application needs to be run by shell on its behalf.
         */
        private final Intent mIntent;

        private RegisterActivityResultReceiverCallback(Intent intent) {
            mIntent = intent;
        }

        /**
         * @see RegisterActivityResultReceiverResponse#call(byte[])
         */
        @Override
        public void call(byte[] serializedIntent) {
            Intent trampolineIntent = bytesToIntent(serializedIntent);
            trampolineIntent.putExtra(EXTRA_START_ACTIVITY_FOR_RESULT_INTENT, mIntent);
            mContext.startActivity(trampolineIntent);
        }
    }

    private final Context mContext;
    private final String mConsumerURL;
    private final IntentReceiverManager mIntentReceiverManager;
    private Db mDb = null;
    private File mDbFile = null;

    public AuthenticationServiceImpl(Context context, Core core, String consumerURL, Shell shell) {
        mContext = context;
        mConsumerURL = consumerURL;
        mIntentReceiverManager = ShellHelper.connectToService(
                core, shell, "mojo:intent_receiver", IntentReceiverManager.MANAGER);
    }

    /**
     * @see AuthenticationService#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {}

    /**
     * @see AuthenticationService#getOAuth2Token(String, String[],
     *      AuthenticationService.GetOAuth2TokenResponse)
     */
    @Override
    public void getOAuth2Token(
            final String username, final String[] scopes, final GetOAuth2TokenResponse callback) {
        if (scopes.length == 0) {
            callback.call(null, "scopes cannot be empty");
            return;
        }
        StringBuilder scope = new StringBuilder("oauth2:");
        for (int i = 0; i < scopes.length; ++i) {
            if (i > 0) {
                scope.append(" ");
            }
            scope.append(scopes[i]);
        }
        try {
            callback.call(GoogleAuthUtil.getToken(mContext, username, scope.toString()), null);
        } catch (final UserRecoverableAuthException e) {
            // If an error occurs that the user can recover from, this exception will contain the
            // intent to run to allow the user to act. This will use the intent manager to start it.
            mIntentReceiverManager.registerActivityResultReceiver(new IntentReceiver() {
                GetOAuth2TokenResponse mPendingCallback = callback;

                @Override
                public void close() {
                    call(null, "User denied the request.");
                }

                @Override
                public void onConnectionError(MojoException e) {
                    call(null, e.getMessage());
                }

                @Override
                public void onIntent(byte[] bytes) {
                    if (mPendingCallback == null) {
                        return;
                    }
                    if (bytes != null) {
                        getOAuth2Token(username, scopes, mPendingCallback);
                    } else {
                        mPendingCallback.call(null, e.getMessage());
                    }
                    mPendingCallback = null;
                }

                private void call(String token, String error) {
                    if (mPendingCallback == null) {
                        return;
                    }
                    mPendingCallback.call(token, error);
                    mPendingCallback = null;
                }
            }, new RegisterActivityResultReceiverCallback(e.getIntent()));
            return;
        } catch (IOException | GoogleAuthException e) {
            // Unrecoverable error.
            callback.call(null, e.getMessage());
        }
    }

    /**
     * @see AuthenticationService#selectAccount(boolean,
     *      AuthenticationService.SelectAccountResponse)
     */
    @Override
    public void selectAccount(boolean returnLastSelected, final SelectAccountResponse callback) {
        if (returnLastSelected) {
            Db db = getDb();
            String username = db.lastSelectedAccounts.get(mConsumerURL);
            if (username != null) {
                try {
                    GoogleAuthUtil.getAccountId(mContext, username);
                    callback.call(username, null);
                    return;
                } catch (final GoogleAuthException | IOException e) {
                }
            }
        }

        if (!isGooglePlayServicesAvailable(mContext)) {
            callback.call(null, "Google Play Services are not available.");
            return;
        }

        String message = null;
        if (mConsumerURL.equals("")) {
            message = "Select an account to use with mojo shell";
        } else {
            message = "Select an account to use with application: " + mConsumerURL;
        }
        Intent accountPickerIntent = AccountPicker.newChooseAccountIntent(
                null, null, ACCOUNT_TYPES, false, message, null, null, null);

        mIntentReceiverManager.registerActivityResultReceiver(new IntentReceiver() {
            SelectAccountResponse mPendingCallback = callback;

            @Override
            public void close() {
                call(null, "User denied the request.");
            }

            @Override
            public void onConnectionError(MojoException e) {
                call(null, e.getMessage());
            }

            @Override
            public void onIntent(byte[] bytes) {
                if (bytes == null) {
                    call(null, "Unable to retrieve user name.");
                    return;
                }
                Intent intent = bytesToIntent(bytes);
                call(intent.getStringExtra(AccountManager.KEY_ACCOUNT_NAME), null);
            }

            private void call(String username, String error) {
                if (mPendingCallback == null) {
                    return;
                }
                mPendingCallback.call(username, error);
                mPendingCallback = null;
                updateDb(username);
            }
        }, new RegisterActivityResultReceiverCallback(accountPickerIntent));
    }

    /**
     * @see AuthenticationService#clearOAuth2Token(String)
     */
    @Override
    public void clearOAuth2Token(String token) {
        try {
            GoogleAuthUtil.clearToken(mContext, token);
        } catch (GoogleAuthException | IOException e) {
            // Nothing to do.
        }
    }

    private static boolean isGooglePlayServicesAvailable(Context context) {
        switch (GooglePlayServicesUtil.isGooglePlayServicesAvailable(context)) {
            case ConnectionResult.SERVICE_MISSING:
            case ConnectionResult.SERVICE_VERSION_UPDATE_REQUIRED:
            case ConnectionResult.SERVICE_DISABLED:
                return false;
            default:
                return true;
        }
    }

    private static Intent bytesToIntent(byte[] bytes) {
        Parcel p = Parcel.obtain();
        p.unmarshall(bytes, 0, bytes.length);
        p.setDataPosition(0);
        return Intent.CREATOR.createFromParcel(p);
    }

    private File getDbFile() {
        if (mDbFile != null) {
            return mDbFile;
        }
        File home = new File(System.getenv("HOME"));
        File configDir = new File(home, ".mojo_authentication");
        configDir.mkdirs();
        mDbFile = new File(configDir, "db");
        return mDbFile;
    }

    private Db getDb() {
        if (mDb != null) {
            return mDb;
        }
        File dbFile = getDbFile();
        if (dbFile.exists()) {
            try {
                int size = (int) dbFile.length();
                try (FileInputStream stream = new FileInputStream(dbFile);
                        FileChannel channel = stream.getChannel()) {
                    // Use mojo serialization to read the database.
                    Db db = Db.deserialize(new Message(
                            channel.map(MapMode.READ_ONLY, 0, size), new ArrayList<Handle>()));
                    if (db.version == VERSION) {
                        mDb = db;
                        return mDb;
                    }
                } catch (DeserializationException e) {
                }
                dbFile.delete();
            } catch (IOException e) {
            }
        }
        mDb = new Db();
        mDb.version = VERSION;
        mDb.lastSelectedAccounts = new HashMap<>();
        return mDb;
    }

    private void updateDb(String username) {
        try {
            Db db = getDb();
            if (username == null) {
                db.lastSelectedAccounts.remove(mConsumerURL);
            } else {
                db.lastSelectedAccounts.put(mConsumerURL, username);
            }
            // Use mojo serialization to persist the database.
            Message m = db.serialize(null);
            File dbFile = getDbFile();
            dbFile.delete();
            try (FileOutputStream stream = new FileOutputStream(dbFile);
                    FileChannel channel = stream.getChannel()) {
                channel.write(m.getData());
            }
        } catch (IOException e) {
        }
    }
}
