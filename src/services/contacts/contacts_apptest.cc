// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/contacts/interfaces/contacts.mojom.h"

namespace contacts {

class ContactAppTest : public mojo::test::ApplicationTestBase {
 public:
  ContactAppTest() : ApplicationTestBase() {}
  ~ContactAppTest() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();
    mojo::ConnectToService(shell(), "mojo:contacts",
                           GetProxy(&contacts_service_));
  }

 protected:
  contacts::ContactsServicePtr contacts_service_;

  DISALLOW_COPY_AND_ASSIGN(ContactAppTest);
};

TEST_F(ContactAppTest, Count) {
  uint64_t count;
  contacts_service_->GetCount(mojo::String(), [&count](uint64_t c) {
    count = c;
    base::MessageLoop::current()->QuitWhenIdle();
  });
  { base::RunLoop().Run(); }
}

TEST_F(ContactAppTest, Get) {
  mojo::Array<ContactPtr> contacts;
  contacts_service_->Get(mojo::String(), 0, 1,
                         [&contacts](mojo::Array<ContactPtr> c) {
                           contacts = c.Pass();
                           base::MessageLoop::current()->QuitWhenIdle();
                         });
  { base::RunLoop().Run(); }
  EXPECT_FALSE(contacts.is_null());
}

TEST_F(ContactAppTest, GetWithFilter) {
  mojo::Array<ContactPtr> contacts;
  contacts_service_->Get("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", 0, 1,
                         [&contacts](mojo::Array<ContactPtr> c) {
                           contacts = c.Pass();
                           base::MessageLoop::current()->QuitWhenIdle();
                         });
  { base::RunLoop().Run(); }
  EXPECT_FALSE(contacts.is_null());
  EXPECT_EQ(0u, contacts.size());
}

TEST_F(ContactAppTest, GetEmails) {
  mojo::Array<mojo::String> emails;

  contacts_service_->GetEmails(0, [&emails](mojo::Array<mojo::String> m) {
    emails = m.Pass();
    base::MessageLoop::current()->QuitWhenIdle();
  });
  { base::RunLoop().Run(); }
  EXPECT_FALSE(emails.is_null());
  EXPECT_EQ(0u, emails.size());
}

TEST_F(ContactAppTest, GetPhoto) {
  mojo::String url;

  contacts_service_->GetPhoto(0, false, [&url](const mojo::String& u) {
    url = u;
    base::MessageLoop::current()->QuitWhenIdle();
  });
  { base::RunLoop().Run(); };
  EXPECT_TRUE(url.is_null());

  contacts_service_->GetPhoto(0, true, [&url](const mojo::String& u) {
    url = u;
    base::MessageLoop::current()->QuitWhenIdle();
  });
  { base::RunLoop().Run(); };
  EXPECT_TRUE(url.is_null());
}

}  // namespace contacts
