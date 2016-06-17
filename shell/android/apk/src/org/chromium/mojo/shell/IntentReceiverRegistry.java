// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.content.Intent;
import android.os.Parcel;

import org.chromium.base.ApplicationStatus;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.intent.IntentReceiver;
import org.chromium.mojo.intent.IntentReceiverManager;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.RunLoop;
import org.chromium.mojo.system.impl.CoreImpl;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
 * Java implementation for services/intent_receiver/intent_receiver.mojom. This class creates
 * intents for privileged mojo applications and routes received intents back to the mojo
 * application.
 */
public class IntentReceiverRegistry
        implements IntentReceiverManager, ServiceFactoryBinder<IntentReceiverManager> {
    private static class LazyHolder {
        private static final IntentReceiverRegistry INSTANCE = new IntentReceiverRegistry();
    }

    /**
     * Returns the instance.
     */
    public static IntentReceiverRegistry getInstance() {
        return LazyHolder.INSTANCE;
    }

    /**
     * The runloop the service runs on.
     */
    private RunLoop mRunLoop = null;
    private final Map<String, IntentReceiver> mReceiversByUuid = new HashMap<>();
    private final Map<IntentReceiver, String> mUuidsByReceiver = new HashMap<>();
    private int mLastRequestCode = 0;

    private IntentReceiverRegistry() {}

    public void onIntentReceived(final Intent intent) {
        String uuid = intent.getAction();
        if (uuid == null) return;
        final IntentReceiver receiver = mReceiversByUuid.get(uuid);
        if (receiver == null) return;
        mRunLoop.postDelayedTask(new Runnable() {
            @Override
            public void run() {
                receiver.onIntent(intentToBytes(intent));
            }
        }, 0);
    }

    public void onActivityResult(final int requestCode, final int resultCode, final Intent data) {
        String uuid = Integer.toString(requestCode);
        if (uuid == null) return;
        final IntentReceiver receiver = mReceiversByUuid.get(uuid);
        if (receiver == null) return;
        mRunLoop.postDelayedTask(new Runnable() {
            @Override
            public void run() {
                if (resultCode == Activity.RESULT_OK) {
                    receiver.onIntent(intentToBytes(data));
                } else {
                    receiver.onIntent(null);
                }
                receiver.close();
            }
        }, 0);
        unregisterReceiver(receiver);
    }

    private static byte[] intentToBytes(Intent intent) {
        Parcel p = Parcel.obtain();
        intent.writeToParcel(p, 0);
        p.setDataPosition(0);
        return p.marshall();
    }

    /**
     * @see IntentReceiverManager#registerIntentReceiver(IntentReceiver,
     *      RegisterIntentReceiverResponse)
     */
    @Override
    public void registerIntentReceiver(
            IntentReceiver receiver, RegisterIntentReceiverResponse callback) {
        registerErrorHandler(receiver);
        String uuid = UUID.randomUUID().toString();
        Intent intent = new Intent(uuid, null, ApplicationStatus.getApplicationContext(),
                IntentReceiverActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mReceiversByUuid.put(uuid, receiver);
        mUuidsByReceiver.put(receiver, uuid);
        callback.call(intentToBytes(intent));
    }

    /**
     * @see IntentReceiverManager#registerActivityResultReceiver(IntentReceiver,
     *      RegisterActivityResultReceiverResponse)
     */
    @Override
    public void registerActivityResultReceiver(
            IntentReceiver receiver, RegisterActivityResultReceiverResponse callback) {
        registerErrorHandler(receiver);
        String uuid;
        // Handle unlikely overflows.
        do {
            ++mLastRequestCode;
            if (mLastRequestCode < 1) {
                mLastRequestCode = 1;
            }
            uuid = Integer.toString(mLastRequestCode);
        } while (mReceiversByUuid.keySet().contains(uuid));
        mReceiversByUuid.put(uuid, receiver);
        mUuidsByReceiver.put(receiver, uuid);
        Intent intent = new Intent(uuid, null, ApplicationStatus.getApplicationContext(),
                IntentReceiverActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.addCategory(IntentReceiverActivity.CATEGORY_START_ACTIVITY_FOR_RESULT);
        callback.call(intentToBytes(intent));
    }

    /**
     * @see IntentReceiverManager#close()
     */
    @Override
    public void close() {}

    /**
     * @see IntentReceiverManager#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {}

    /**
     * @see ServiceFactoryBinder#bind(InterfaceRequest)
     */
    @Override
    public void bind(InterfaceRequest<IntentReceiverManager> request) {
        if (mRunLoop == null) {
            mRunLoop = CoreImpl.getInstance().getCurrentRunLoop();
        }
        assert mRunLoop == CoreImpl.getInstance().getCurrentRunLoop();
        IntentReceiverManager.MANAGER.bind(this, request);
    }

    /**
     * @see ServiceFactoryBinder#getInterfaceName()
     */
    @Override
    public String getInterfaceName() {
        return IntentReceiverManager.MANAGER.getName();
    }

    private void unregisterReceiver(IntentReceiver receiver) {
        IntentReceiverRegistry registry = getInstance();
        String uuid = registry.mUuidsByReceiver.get(receiver);
        if (uuid != null) {
            registry.mUuidsByReceiver.remove(receiver);
            registry.mReceiversByUuid.remove(uuid);
        }
    }

    private void registerErrorHandler(final IntentReceiver receiver) {
        if (receiver instanceof IntentReceiver.Proxy) {
            IntentReceiver.Proxy proxy = (IntentReceiver.Proxy) receiver;
            proxy.getProxyHandler().setErrorHandler(new ConnectionErrorHandler() {

                @Override
                public void onConnectionError(MojoException e) {
                    unregisterReceiver(receiver);
                }
            });
        }
    }
}
