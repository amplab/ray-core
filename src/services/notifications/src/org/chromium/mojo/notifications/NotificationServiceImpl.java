// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.notifications;

import android.app.ActivityManager;
import android.app.NotificationManager;
import android.content.Context;
import android.util.SparseArray;

import org.chromium.mojo.application.ShellHelper;
import org.chromium.mojo.intent.IntentReceiverManager;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.mojo.Shell;
import org.chromium.mojom.notifications.Notification;
import org.chromium.mojom.notifications.NotificationClient;
import org.chromium.mojom.notifications.NotificationData;
import org.chromium.mojom.notifications.NotificationService;

import java.util.List;

/**
 * Android implementation of Notifications.
 */
class NotificationServiceImpl implements NotificationService,
                                         NotificationBuilder.NotificationActionListener,
                                         NotificationBuilder.NotificationReadyListener {
    private final NotificationManager mNotificationManager;
    private final SparseArray<NotificationImpl> mNotificationMap;
    private final SparseArray<NotificationClient> mNotificationClientMap;

    /**
     * This tag is used when notifying and cancelling notifications to ensure the notifications
     * being notified and cancelled are unique to this notification service instance.
     */
    private final String mNotificationManagerTag;

    private final NotificationBuilder mNotificationBuilder;
    private final ActivityManager mActivityManager;

    private int mNextNotificationId;
    private ActivityManager.AppTask mAppTask;

    NotificationServiceImpl(
            Context context, Core core, Shell shell, int notificationIconResourceId) {
        mNotificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        mNotificationMap = new SparseArray<NotificationImpl>();
        mNotificationClientMap = new SparseArray<NotificationClient>();
        mNotificationManagerTag = toString();
        IntentReceiverManager intentReceiverManager = ShellHelper.connectToService(
                core, shell, "mojo:intent_receiver", IntentReceiverManager.MANAGER);
        mNotificationBuilder = new NotificationBuilder(
                context, intentReceiverManager, notificationIconResourceId, this, this);
        mActivityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.AppTask> tasks = mActivityManager.getAppTasks();
        // Associate the service instance with the current top task of the shell
        // application. All notifications created by this instance will be associated with
        // this task too, and the task will be foregrounded when any of the notifications
        // is selected.
        mAppTask = tasks.isEmpty() ? null : tasks.get(0);
        mNextNotificationId = 1;
    }

    // NotificationService implementation.
    @Override
    public void close() {}

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void post(NotificationData notificationData, NotificationClient notificationClient,
            org.chromium.mojo.bindings.InterfaceRequest<Notification> request) {
        final int newNotificationId = getNewNotificationId();
        mNotificationClientMap.put(newNotificationId, notificationClient);
        NotificationImpl notification = new NotificationImpl(this, newNotificationId);
        mNotificationMap.put(newNotificationId, notification);
        Notification.MANAGER.bind(notification, request);
        postOrUpdateNotification(newNotificationId, notificationData);
    }

    // NotificationBuilder.NotificationActionListener implementation.
    @Override
    public void onNotificationSelected(int notificationId) {
        NotificationClient client = mNotificationClientMap.get(notificationId);
        if (client != null) {
            // If the selected task is not active anymore, choose the most recent one and associate
            // this service with it.
            if (mAppTask == null || mAppTask.getTaskInfo().id == -1) {
                List<ActivityManager.AppTask> tasks = mActivityManager.getAppTasks();
                mAppTask = tasks.isEmpty() ? null : tasks.get(0);
            }
            if (mAppTask != null) {
                mAppTask.moveToFront();
            }
            client.onSelected();
        }
        // Since autoCancel is set to true (@see NotificationBuilder#build(int, NotificationData)),
        // the notification no longer exists at this point. Clean it up.
        cleanUpNotification(notificationId);
    }

    @Override
    public void onNotificationDismissed(int notificationId) {
        NotificationClient client = mNotificationClientMap.get(notificationId);
        if (client != null) {
            client.onDismissed();
        }
        // Since the notification was dismissed it can no longer be interacted with. Clean it up.
        cleanUpNotification(notificationId);
    }

    // NotificationBuilder.NotificationReadyListener implementation.
    @Override
    public void onNotificationReady(int notificationId, android.app.Notification notification) {
        // Only allow notification of ids we know about. This will filter out situations where a
        // cancel quickly follows a post or update before the notification is finished being
        // created.
        if (mNotificationMap.get(notificationId) != null) {
            mNotificationManager.notify(mNotificationManagerTag, notificationId, notification);
        }
    }

    void cancel(int notificationId) {
        mNotificationManager.cancel(mNotificationManagerTag, notificationId);
        // Since we're cancelling the notification, the notification can no longer be interacted
        // with. Clean it up.
        cleanUpNotification(notificationId);
    }

    /**
     * Posts a notification to the notification manager once built, or updates that notification if
     * it already exists.
     */
    void postOrUpdateNotification(int notificationId, NotificationData notificationData) {
        mNotificationBuilder.build(notificationId, notificationData);
    }

    private int getNewNotificationId() {
        return mNextNotificationId++;
    }

    private void cleanUpNotification(int notificationId) {
        NotificationClient client = mNotificationClientMap.get(notificationId);
        if (client != null) {
            client.close();
        }

        mNotificationClientMap.remove(notificationId);

        NotificationImpl notification = mNotificationMap.get(notificationId);
        if (notification != null) {
            notification.invalidate();
        }
        mNotificationMap.remove(notificationId);
    }
}
