// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.notifications;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Parcel;

import org.chromium.mojo.intent.IntentReceiver;
import org.chromium.mojo.intent.IntentReceiverManager;
import org.chromium.mojo.intent.IntentReceiverManager.RegisterIntentReceiverResponse;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.notifications.NotificationData;

/**
 * Builds notifications using Intents from the IntentReceiverManager.
 */
class NotificationBuilder {
    interface NotificationReadyListener {
        void onNotificationReady(int notificationId, Notification notification);
    }

    interface NotificationActionListener {
        void onNotificationSelected(int notificationId);
        void onNotificationDismissed(int notificationId);
    }

    private enum Action { SELECT, DELETE }

    private final Context mContext;
    private final IntentReceiverManager mIntentReceiverManager;
    private final int mNotificationIconResourceId;
    private final NotificationActionListener mNotificationActionListener;
    private final NotificationReadyListener mNotificationReadyListener;

    NotificationBuilder(Context context, IntentReceiverManager intentReceiverManager,
            int notificationIconResourceId, NotificationActionListener notificationActionListener,
            NotificationReadyListener notificationReadyListener) {
        mContext = context;
        mIntentReceiverManager = intentReceiverManager;
        mNotificationIconResourceId = notificationIconResourceId;
        mNotificationActionListener = notificationActionListener;
        mNotificationReadyListener = notificationReadyListener;
    }

    void build(int notificationId, NotificationData notificationData) {
        Notification.Builder notificationBuilder = new Notification.Builder(mContext);
        notificationBuilder.setContentTitle(notificationData.title);
        notificationBuilder.setContentText(notificationData.text);
        notificationBuilder.setSmallIcon(mNotificationIconResourceId);
        notificationBuilder.setAutoCancel(true);
        if (notificationData.playSound) {
            Uri alarmSound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
            notificationBuilder.setSound(alarmSound);
        }
        if (notificationData.vibrate) {
            long[] vibratePattern = {500, 500, 500, 500, 500, 500, 500, 500, 500};
            notificationBuilder.setVibrate(vibratePattern);
        }
        if (notificationData.setLights) {
            notificationBuilder.setLights(Notification.COLOR_DEFAULT, 500, 500);
        }

        populatePendingIntent(Action.SELECT, notificationId, notificationBuilder);
    }

    void populatePendingIntent(
            Action action, int notificationId, Notification.Builder notificationBuilder) {
        mIntentReceiverManager.registerIntentReceiver(
                new ActionIntentReceiver(action, notificationId),
                new RegisterActionIntentReceiverResponse(
                        action, notificationId, notificationBuilder));
    }

    class RegisterActionIntentReceiverResponse implements RegisterIntentReceiverResponse {
        private final Action mAction;
        private final int mNotificationId;
        private final Notification.Builder mNotificationBuilder;

        RegisterActionIntentReceiverResponse(
                Action action, int notificationId, Notification.Builder notificationBuilder) {
            mAction = action;
            mNotificationId = notificationId;
            mNotificationBuilder = notificationBuilder;
        }

        @Override
        public void call(byte[] serializedIntent) {
            PendingIntent pendingIntent =
                    PendingIntent.getActivity(mContext, 0, bytesToIntent(serializedIntent), 0);

            switch (mAction) {
                case SELECT:
                    mNotificationBuilder.setContentIntent(pendingIntent);
                    populatePendingIntent(Action.DELETE, mNotificationId, mNotificationBuilder);
                    break;
                case DELETE:
                    mNotificationBuilder.setDeleteIntent(pendingIntent);
                    mNotificationReadyListener.onNotificationReady(
                            mNotificationId, mNotificationBuilder.build());
                    break;
            }
        }

        private Intent bytesToIntent(byte[] bytes) {
            Parcel p = Parcel.obtain();
            p.unmarshall(bytes, 0, bytes.length);
            p.setDataPosition(0);
            return Intent.CREATOR.createFromParcel(p);
        }
    }

    class ActionIntentReceiver implements IntentReceiver {
        private final Action mAction;
        private final int mNotificationId;

        ActionIntentReceiver(Action action, int notificationId) {
            mAction = action;
            mNotificationId = notificationId;
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}

        @Override
        public void onIntent(byte[] bytes) {
            switch (mAction) {
                case SELECT:
                    mNotificationActionListener.onNotificationSelected(mNotificationId);
                    break;
                case DELETE:
                    mNotificationActionListener.onNotificationDismissed(mNotificationId);
                    break;
            }
        }
    }
}
