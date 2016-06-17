// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/ui/views/interfaces/view_manager.mojom.h"
#include "mojo/services/ui/views/interfaces/views.mojom.h"
#include "services/ui/view_manager/tests/mock_view_associate.h"
#include "services/ui/view_manager/tests/view_manager_test_base.h"

namespace view_manager {
namespace test {

class MockViewListener : public mojo::ui::ViewListener {
 public:
  MockViewListener() {}
  ~MockViewListener() override {}

  void OnInvalidation(mojo::ui::ViewInvalidationPtr invalidation,
                      const OnInvalidationCallback& callback) override {
    callback.Run();
  }
};

class ViewManagerTest : public ViewManagerTestBase {
 public:
  ViewManagerTest() {}
  ~ViewManagerTest() override {}

  void SetUp() override {
    ViewManagerTestBase::SetUp();

    // Connect to view manager
    mojo::ConnectToService(shell(), "mojo:view_manager_service",
                           mojo::GetProxy(&view_manager_));
  }

 protected:
  mojo::ui::ViewManagerPtr view_manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewManagerTest);
};

TEST_F(ViewManagerTest, CreateAViewManagerAndView) {
  // Create and bind a mock view listener
  mojo::ui::ViewListenerPtr view_listener;
  MockViewListener mock_view_listener;
  mojo::Binding<mojo::ui::ViewListener> view_listener_binding(
      &mock_view_listener, mojo::GetProxy(&view_listener));

  // Create a view
  mojo::ui::ViewPtr view;
  mojo::ui::ViewOwnerPtr client_view_owner;
  view_manager_->CreateView(mojo::GetProxy(&view),
                            mojo::GetProxy(&client_view_owner),
                            view_listener.Pass(), "test_view");

  // Call View::GetToken. Check that you get the callback.
  int view_token_callback_invokecount = 0;
  auto view_token_callback = [&view_token_callback_invokecount](
      mojo::ui::ViewTokenPtr token) { view_token_callback_invokecount++; };

  EXPECT_EQ(0, view_token_callback_invokecount);
  view->GetToken(view_token_callback);

  KICK_MESSAGE_LOOP_WHILE(view_token_callback_invokecount != 1);

  EXPECT_EQ(1, view_token_callback_invokecount);
}

TEST_F(ViewManagerTest, ConnectAMockViewAssociate) {
  // Create and bind a MockViewAssociate
  mojo::InterfaceHandle<mojo::ui::ViewAssociate> associate;
  MockViewAssociate mock_view_associate;
  mojo::Binding<mojo::ui::ViewAssociate> view_associate_binding(
      &mock_view_associate, mojo::GetProxy(&associate));

  // Call ViewManager::RegisterViewAssociate. MockViewAssociate::Connect
  // should be called back
  EXPECT_EQ(0, mock_view_associate.connect_invokecount);
  mojo::ui::ViewAssociateOwnerPtr view_associate_owner;
  view_manager_->RegisterViewAssociate(associate.Pass(),
                                       mojo::GetProxy(&view_associate_owner),
                                       "test_view_associate");

  KICK_MESSAGE_LOOP_WHILE(mock_view_associate.connect_invokecount != 1);

  EXPECT_EQ(1, mock_view_associate.connect_invokecount);
}

TEST_F(ViewManagerTest, DisconnectAMockViewAssociate) {
  mojo::ui::ViewAssociateOwnerPtr view_associate_owner;
  int owner_connection_error_callback_invokecount = 0;

  {
    // Create and bind a MockViewAssociate
    mojo::InterfaceHandle<mojo::ui::ViewAssociate> associate;
    MockViewAssociate mock_view_associate;
    mojo::Binding<mojo::ui::ViewAssociate> view_associate_binding(
        &mock_view_associate, mojo::GetProxy(&associate));

    // Call ViewManager::RegisterViewAssociate. MockViewAssociate::Connect
    // should be called back
    EXPECT_EQ(0, mock_view_associate.connect_invokecount);

    view_manager_->RegisterViewAssociate(associate.Pass(),
                                         mojo::GetProxy(&view_associate_owner),
                                         "test_view_associate_xyz");

    // set a callback for errors
    view_associate_owner.set_connection_error_handler(
        // use lambda function as callback
        [&owner_connection_error_callback_invokecount]() {
          owner_connection_error_callback_invokecount++;
        });

    KICK_MESSAGE_LOOP_WHILE(mock_view_associate.connect_invokecount != 1);

    EXPECT_EQ(1, mock_view_associate.connect_invokecount);

    EXPECT_EQ(0, owner_connection_error_callback_invokecount);
  }

  // mock_view_associate is out of scope, should be destroyed
  // we expect to get a connection error from the owner
  KICK_MESSAGE_LOOP_WHILE(owner_connection_error_callback_invokecount != 1)

  EXPECT_EQ(1, owner_connection_error_callback_invokecount);
}

TEST_F(ViewManagerTest, DisconnectAViewAssociateOwner) {
  // Create and bind a MockViewAssociate
  mojo::InterfaceHandle<mojo::ui::ViewAssociate> associate;
  MockViewAssociate mock_view_associate;
  mojo::Binding<mojo::ui::ViewAssociate> view_associate_binding(
      &mock_view_associate, mojo::GetProxy(&associate));

  // set a callback for errors
  int connection_error_callback_invokecount = 0;
  view_associate_binding.set_connection_error_handler(
      // use lambda function as callback
      [&connection_error_callback_invokecount]() {
        connection_error_callback_invokecount++;
      });

  {
    mojo::ui::ViewAssociateOwnerPtr view_associate_owner;

    // Call ViewManager::RegisterViewAssociate. MockViewAssociate::Connect
    // should be called back
    EXPECT_EQ(0, mock_view_associate.connect_invokecount);

    view_manager_->RegisterViewAssociate(associate.Pass(),
                                         mojo::GetProxy(&view_associate_owner),
                                         "test_view_associate_xyz");

    KICK_MESSAGE_LOOP_WHILE(mock_view_associate.connect_invokecount != 1);

    EXPECT_EQ(1, mock_view_associate.connect_invokecount);

    EXPECT_EQ(0, connection_error_callback_invokecount);
  }

  // view_associate_owner is out of scope, should be destroyed
  // we expect to get a connection error from the view associate
  KICK_MESSAGE_LOOP_WHILE(connection_error_callback_invokecount != 1)

  EXPECT_EQ(1, connection_error_callback_invokecount);
}

}  // namespace test
}  // namespace view_manager
