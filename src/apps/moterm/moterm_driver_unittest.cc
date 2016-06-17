// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/moterm_driver.h"

#include <deque>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A test |MotermDriver::Client| that just records the notifications it
// receives. When |OnDestroyed()| gets called, it'll also quit the current
// message loop.
class TestClient : public MotermDriver::Client {
 public:
  struct Event {
    enum class Type { DATA_RECEIVED, CLOSED, DESTROYED };

    Event(Type type) : type(type) {}
    Event(Type type, const std::string& data) : type(type), data(data) {}
    ~Event() {}

    Type type;
    std::string data;
  };

  TestClient() {}
  ~TestClient() {}

  std::deque<Event>& events() { return events_; }

 private:
  // |MotermDriver::Client|:
  void OnDataReceived(const void* bytes, size_t num_bytes) override {
    events_.push_back(
        Event(Event::Type::DATA_RECEIVED,
              std::string(static_cast<const char*>(bytes), num_bytes)));
  }

  void OnClosed() override { events_.push_back(Event(Event::Type::CLOSED)); }

  void OnDestroyed() override {
    events_.push_back(Event(Event::Type::DESTROYED));
    base::MessageLoop::current()->Quit();
  }

  std::deque<Event> events_;

  DISALLOW_COPY_AND_ASSIGN(TestClient);
};

using MotermDriverTest = mojo::test::ApplicationTestBase;

// This spins the message loop until the callback is run. Note that we can't
// just wait for the response message, since we need to give a chance for the
// driver to handle the message.
void FileWriteString(mojo::files::File* file, const std::string& s) {
  std::vector<uint8_t> bytes_to_write;
  for (size_t i = 0; i < s.size(); i++)
    bytes_to_write.push_back(static_cast<uint8_t>(s[i]));
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  uint32_t num_bytes_written = 0;
  file->Write(mojo::Array<uint8_t>::From(bytes_to_write), 0,
              mojo::files::Whence::FROM_CURRENT,
              [&error, &num_bytes_written](mojo::files::Error e, uint32_t n) {
                error = e;
                num_bytes_written = n;
                base::MessageLoop::current()->Quit();
              });
  base::MessageLoop::current()->Run();
  EXPECT_EQ(mojo::files::Error::OK, error);
  EXPECT_EQ(bytes_to_write.size(), num_bytes_written);
}

// (See the comments above |FileWriteString()|.)
void FileClose(mojo::files::File* file) {
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  file->Close([&error](mojo::files::Error e) {
    error = e;
    base::MessageLoop::current()->Quit();
  });
  base::MessageLoop::current()->Run();
  EXPECT_EQ(mojo::files::Error::OK, error);
}

// (See the comments above |FileWriteString()|.)
std::string FileReadString(mojo::files::File* file, uint32_t max_bytes) {
  mojo::files::Error error = mojo::files::Error::INTERNAL;
  mojo::Array<uint8_t> bytes_read;
  file->Read(
      10, 0, mojo::files::Whence::FROM_CURRENT,
      [&error, &bytes_read](mojo::files::Error e, mojo::Array<uint8_t> b) {
        error = e;
        bytes_read = b.Pass();
        base::MessageLoop::current()->Quit();
      });
  base::MessageLoop::current()->Run();
  EXPECT_EQ(mojo::files::Error::OK, error);
  if (!bytes_read.size())
    return std::string();
  return std::string(reinterpret_cast<const char*>(&bytes_read[0]),
                     bytes_read.size());
}

TEST_F(MotermDriverTest, BasicWritesToTerminal) {
  TestClient client;
  mojo::files::FilePtr file;
  base::WeakPtr<MotermDriver> driver =
      MotermDriver::Create(&client, GetProxy(&file));
  ASSERT_TRUE(driver);

  // Write a simple string.
  std::string hello("hello");
  FileWriteString(file.get(), hello);
  // The client should have gotten the data.
  ASSERT_EQ(1u, client.events().size());
  TestClient::Event event = client.events().front();
  client.events().pop_front();
  EXPECT_EQ(TestClient::Event::Type::DATA_RECEIVED, event.type);
  EXPECT_EQ(hello, event.data);

  // With the default settings, it should transform \n to \r\n.
  FileWriteString(file.get(), "world\n");
  // The client should have gotten the data.
  ASSERT_EQ(1u, client.events().size());
  event = client.events().front();
  client.events().pop_front();
  EXPECT_EQ(TestClient::Event::Type::DATA_RECEIVED, event.type);
  EXPECT_EQ(std::string("world\r\n"), event.data);

  FileClose(file.get());
  // The client should have gotten the closed event.
  ASSERT_EQ(1u, client.events().size());
  event = client.events().front();
  client.events().pop_front();
  EXPECT_EQ(TestClient::Event::Type::CLOSED, event.type);
  // The driver should still be alive.
  EXPECT_TRUE(driver);

  // Actually destroying the file (and spinning the message loop) should kill
  // the driver.
  file.reset();
  base::MessageLoop::current()->Run();
  EXPECT_FALSE(driver);
  // The client should have gotten the destroyed event.
  ASSERT_EQ(1u, client.events().size());
  event = client.events().front();
  client.events().pop_front();
  EXPECT_EQ(TestClient::Event::Type::DESTROYED, event.type);
}

TEST_F(MotermDriverTest, BasicReadsFromTerminal) {
  TestClient client;
  mojo::files::FilePtr file;
  base::WeakPtr<MotermDriver> driver =
      MotermDriver::Create(&client, GetProxy(&file));
  ASSERT_TRUE(driver);

  // Kick off two reads. They should be satisfied in order. (Don't use
  // |FileReadString()| here, since we want to queue up the reads before calling
  // |SendData()|.)
  mojo::files::Error error1 = mojo::files::Error::INTERNAL;
  mojo::Array<uint8_t> bytes_read1;
  file->Read(
      4, 0, mojo::files::Whence::FROM_CURRENT,
      [&error1, &bytes_read1](mojo::files::Error e, mojo::Array<uint8_t> b) {
        error1 = e;
        bytes_read1 = b.Pass();
      });
  // Only quit the message loop on getting the response to the second.
  mojo::files::Error error2 = mojo::files::Error::INTERNAL;
  mojo::Array<uint8_t> bytes_read2;
  file->Read(
      100, 0, mojo::files::Whence::FROM_CURRENT,
      [&error2, &bytes_read2](mojo::files::Error e, mojo::Array<uint8_t> b) {
        error2 = e;
        bytes_read2 = b.Pass();
        base::MessageLoop::current()->Quit();
      });
  // They'll only be satisfied on input from the terminal. (We need to send a
  // \n (or a \r), since by default we start off in canonical mode.)
  std::string data("hello\nworld");
  driver->SendData(data.data(), data.size());
  base::MessageLoop::current()->Run();
  EXPECT_EQ(mojo::files::Error::OK, error1);
  EXPECT_EQ(4u, bytes_read1.size());
  EXPECT_EQ(std::string("hell"),
            std::string(reinterpret_cast<const char*>(&bytes_read1[0]),
                        bytes_read1.size()));
  EXPECT_EQ(mojo::files::Error::OK, error2);
  EXPECT_EQ(2u, bytes_read2.size());
  // (We shouldn't get the "world" yet.)
  EXPECT_EQ(std::string("o\n"),
            std::string(reinterpret_cast<const char*>(&bytes_read2[0]),
                        bytes_read2.size()));

  // Now send a \n.
  driver->SendData("\n", 1);
  // And then do a read.
  std::string result = FileReadString(file.get(), 100);
  // It should now get the "world" (and the \n).
  EXPECT_EQ(std::string("world\n"), result);

  // A bunch of stuff should have been echoed to the terminal. (Don't worry
  // about what was "displayed" here -- we'll test that separately.)
  for (const auto& event : client.events())
    EXPECT_EQ(TestClient::Event::Type::DATA_RECEIVED, event.type);
  client.events().clear();

  file.reset();
  base::MessageLoop::current()->Run();
  EXPECT_FALSE(driver);
  // The client should have gotten the destroyed event.
  ASSERT_EQ(1u, client.events().size());
  TestClient::Event event = client.events().front();
  client.events().pop_front();
  EXPECT_EQ(TestClient::Event::Type::DESTROYED, event.type);
}

TEST_F(MotermDriverTest, BasicLineEditing) {
  TestClient client;
  mojo::files::FilePtr file;
  base::WeakPtr<MotermDriver> driver =
      MotermDriver::Create(&client, GetProxy(&file));
  ASSERT_TRUE(driver);

  // The default erase character is DEL (\x7f). (In case you're wondering, the
  // string literal is split to avoid an "out of range" hex constant.)
  std::string data(
      "abde\x7f\x7f"
      "cdef\n\x7f"
      "124\x7f"
      "345\r\nXYZ");
  driver->SendData(data.data(), data.size());
  std::string result = FileReadString(file.get(), 100);
  EXPECT_EQ(std::string("abcdef\n"), result);
  result = FileReadString(file.get(), 100);
  // By default, ICRNL is set, so \r gets transformed to \n.
  EXPECT_EQ(std::string("12345\n"), result);
  result = FileReadString(file.get(), 100);
  EXPECT_EQ(std::string("\n"), result);

  // A bunch of stuff should have been echoed to the terminal. The driver has
  // some discretion about how it splits up the stuff (into
  // |OnDataReceived()|s), so just concatenate the contents and verify that.
  std::string echoed;
  for (const auto& event : client.events()) {
    EXPECT_EQ(TestClient::Event::Type::DATA_RECEIVED, event.type);
    EXPECT_FALSE(event.data.empty());
    echoed += event.data;
  }
  client.events().clear();
  // Note: Erases should yield BS (\x08), space, BS (unless at the beginning of
  // a line). Note that the "XYZ" hasn't been sent to us yet, but should already
  // be echoed.
  EXPECT_EQ(std::string(
                "abde\x08 \x08\x08 \x08"
                "cdef\r\n124\x08 \x08"
                "345\r\n\r\nXYZ"),
            echoed);

  // Here, detach rather than doing destroying the other end.
  // TODO(vtl): Verify that the driver's end of the message pipe is closed
  // (e.g., by checking that |file| detects that its peer is closed).
  driver->Detach();
  EXPECT_FALSE(driver);
  // The client shouldn't have received anything.
  EXPECT_TRUE(client.events().empty());
}

}  // namespace
