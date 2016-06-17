// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "mojo/common/binding_set.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/services/notifications/interfaces/notifications.mojom.h"

namespace examples {

static const base::TimeDelta kDefaultMessageDelay =
    base::TimeDelta::FromMilliseconds(3000);

class NotificationGeneratorApp : public mojo::ApplicationImplBase,
                                 public notifications::NotificationClient {
 public:
  NotificationGeneratorApp() {}

  ~NotificationGeneratorApp() override {}

  // mojo::ApplicationImplBase implementation.
  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:notifications",
                           GetProxy(&notification_service_));
    PostFirstNotification();
  }

  // notifications::NotificationClient implementation.
  void OnSelected() override {
    PostNotification("You selected a notification!",
                     "Have you dismissed one yet?", &select_notification_);
  }

  void OnDismissed() override {
    PostNotification("You dismissed a notification!",
                     "Have you selected one yet?", &dismissed_notification_);
  }

  void PostFirstNotification() {
    PostNotification("First notification", "Next: Second will be created",
                     &first_notification_);
    PostDelayed(base::Bind(&NotificationGeneratorApp::PostSecondNotification,
                           base::Unretained(this)));
  }

  void PostSecondNotification() {
    PostNotification("Second notification", "Next: First will be updated",
                     &second_notification_);
    PostDelayed(base::Bind(&NotificationGeneratorApp::UpdateFirstNotification,
                           base::Unretained(this)));
  }

  void PostNotification(const char* title,
                        const char* text,
                        notifications::NotificationPtr* notification) {
    mojo::InterfaceHandle<notifications::NotificationClient>
        notification_client;
    auto request = mojo::GetProxy(&notification_client);
    client_bindings_.AddBinding(this, request.Pass());
    notification_service_->Post(CreateNotificationData(title, text).Pass(),
                                std::move(notification_client),
                                GetProxy(notification));
  }

  void UpdateFirstNotification() {
    first_notification_->Update(
        CreateNotificationData("First notification updated",
                               "Next: both cancelled; repeat").Pass());
    PostDelayed(base::Bind(&NotificationGeneratorApp::CancelSecondNotification,
                           base::Unretained(this)));
  }

  void CancelSecondNotification() {
    second_notification_->Cancel();
    PostDelayed(base::Bind(&NotificationGeneratorApp::CancelFirstNotification,
                           base::Unretained(this)));
  }

  void CancelFirstNotification() {
    first_notification_->Cancel();
    PostDelayed(base::Bind(&NotificationGeneratorApp::PostFirstNotification,
                           base::Unretained(this)));
  }

  notifications::NotificationDataPtr CreateNotificationData(const char* title,
                                                            const char* text) {
    notifications::NotificationDataPtr notification_data =
        notifications::NotificationData::New();
    notification_data->title = mojo::String(title);
    notification_data->text = mojo::String(text);
    return notification_data;
  }

  void PostDelayed(base::Closure closure) {
    base::MessageLoop::current()->PostDelayedTask(FROM_HERE, closure,
                                                  kDefaultMessageDelay);
  }

 private:
  notifications::NotificationServicePtr notification_service_;
  mojo::BindingSet<notifications::NotificationClient> client_bindings_;
  notifications::NotificationPtr first_notification_;
  notifications::NotificationPtr second_notification_;
  notifications::NotificationPtr dismissed_notification_;
  notifications::NotificationPtr select_notification_;

  DISALLOW_COPY_AND_ASSIGN(NotificationGeneratorApp);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  examples::NotificationGeneratorApp notification_generator_app;
  return mojo::RunApplication(application_request, &notification_generator_app);
}
