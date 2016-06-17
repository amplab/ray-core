// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/notifications/interfaces/notifications.mojom.h"

namespace mojo {
namespace {

// Tests the Notification mojo service on android.
class NotificationsApplicationTest : public test::ApplicationTestBase,
                                     public notifications::NotificationClient {
 public:
  NotificationsApplicationTest() : ApplicationTestBase(), binding_(this) {}
  ~NotificationsApplicationTest() override {}

  // notifications::NotificationClient implementation.
  void OnSelected() override {}

  void OnDismissed() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    ConnectToService(shell(), "mojo:notifications",
                     GetProxy(&notification_service_));
  }

  notifications::NotificationServicePtr notification_service_;
  mojo::Binding<notifications::NotificationClient> binding_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(NotificationsApplicationTest);
};

TEST_F(NotificationsApplicationTest, PostAndCancelNotification) {
  notifications::NotificationDataPtr notification_data =
      notifications::NotificationData::New();

  notifications::NotificationClientPtr notification_client;
  auto request = mojo::GetProxy(&notification_client);
  binding_.Bind(request.Pass());

  notifications::NotificationPtr notification;
  notification_service_->Post(notification_data.Pass(),
                              notification_client.Pass(),
                              GetProxy(&notification));
  notification->Cancel();
}

TEST_F(NotificationsApplicationTest, PostUpdateAndCancelNotification) {
  notifications::NotificationDataPtr notification_data =
      notifications::NotificationData::New();

  notifications::NotificationClientPtr notification_client;
  auto request = mojo::GetProxy(&notification_client);
  binding_.Bind(request.Pass());

  notifications::NotificationPtr notification;
  notification_service_->Post(notification_data.Pass(),
                              notification_client.Pass(),
                              GetProxy(&notification));
  notifications::NotificationDataPtr updated_notification_data =
      notifications::NotificationData::New();
  notification->Update(updated_notification_data.Pass());
  notification->Cancel();
}

}  // namespace
}  // namespace mojo
