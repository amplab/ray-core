// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/lib/array_serialization.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/interfaces/bindings/tests/rect.mojom.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

class StructSerializationAPITest : public testing::Test {
 public:
  // We Serialize and Deserialize the given |Type|, and expect the given
  // serialization warnings.  We also validate (and expect a given error) in
  // between serialization and deserialization.
  template <typename Type>
  void SerializeAndDeserialize(
      Type* val,
      mojo::internal::ValidationError expected_validation_error) {
    size_t bytes_written = 0;
    size_t num_bytes = val->GetSerializedSize();
    std::vector<uint8_t> bytes(num_bytes + 1);

    // Last byte is a magic value, helps catch a buffer overflow for
    // serialization.
    bytes[num_bytes] = 170;
    val->Serialize(bytes.data(), num_bytes, &bytes_written);
    EXPECT_EQ(170u, bytes[num_bytes]);
    EXPECT_EQ(num_bytes, bytes_written);

    mojo::internal::BoundsChecker bounds_checker(bytes.data(), num_bytes, 0);
    auto actual_validation_error =
        Type::Data_::Validate(bytes.data(), &bounds_checker, nullptr);
    EXPECT_EQ(expected_validation_error, actual_validation_error);

    Type out_val;
    bool deserialize_ret = out_val.Deserialize(bytes.data(), bytes.size());
    if (actual_validation_error == mojo::internal::ValidationError::NONE) {
      EXPECT_TRUE(val->Equals(out_val));
    }
    EXPECT_EQ(actual_validation_error == mojo::internal::ValidationError::NONE,
              deserialize_ret);
  }
};

TEST_F(StructSerializationAPITest, GetSerializedSize) {
  Rect rect;
  // 8 byte struct header
  // + 16 bytes worth of fields
  EXPECT_EQ(24u, rect.GetSerializedSize());

  HandleStruct handle_struct;
  // 8 byte struct header
  // + 4 byte handle
  // + 8 bytes for offset/pointer for Array.
  // + 0 bytes for uninitialized Array.
  // + 4-byte to make the struct 8-byte aligned.
  EXPECT_EQ(24u, handle_struct.GetSerializedSize());

  // + 8 bytes for initialized array, 0-sized array.
  handle_struct.array_h = mojo::Array<mojo::ScopedMessagePipeHandle>::New(0);
  EXPECT_EQ(32u, handle_struct.GetSerializedSize());

  // + 4 bytes for array of size 1.
  // + 4 more bytes to make the array serialization 8-byte aligned.
  handle_struct.array_h = mojo::Array<mojo::ScopedMessagePipeHandle>::New(1);
  EXPECT_EQ(16u, GetSerializedSize_(handle_struct.array_h));
  EXPECT_EQ(40u, handle_struct.GetSerializedSize());
}

TEST_F(StructSerializationAPITest, BasicStructSerialization) {
  {
    SCOPED_TRACE("Rect");
    Rect rect;
    rect.x = 123;
    rect.y = 456;
    rect.width = 789;
    rect.height = 999;
    SerializeAndDeserialize(&rect, mojo::internal::ValidationError::NONE);
  }

  {
    SCOPED_TRACE("DefaultFieldValues");
    DefaultFieldValues default_values;
    SerializeAndDeserialize(&default_values,
                            mojo::internal::ValidationError::NONE);
  }

  {
    SCOPED_TRACE("NoDefaultFieldValues.Serialize() should fail");
    NoDefaultFieldValues nd;
    nd.f0 = true;
    nd.f23 = mojo::Array<mojo::String>::New(10);

    char buf[1000] = {};
    EXPECT_FALSE(nd.Serialize(buf, sizeof(buf)));

    size_t bytes_written = 0;
    EXPECT_FALSE(nd.Serialize(buf, sizeof(buf), &bytes_written));
    EXPECT_EQ(160UL, bytes_written);
    // The Serialize() shouldn't get around to reserving space for the |f23|
    // array field.
    EXPECT_LT(bytes_written, nd.GetSerializedSize());
  }
}

// This tests serialization of handles -- These should be deaths or
TEST_F(StructSerializationAPITest, HandlesSerialization) {
  {
    SCOPED_TRACE("Uninitialized Array");
    HandleStruct handle_struct;
    // TODO(vardhan):  Once handles and pointers are encoded inline with
    // serialization, validation should fail at
    // ValidationError::UNEXPECTED_NULL_POINTER (which happens after the
    // ValidationError::ILLEGAL_HANDLE error, which shouldn't happen since
    // handles will be encoded even on failures).
    SerializeAndDeserialize(&handle_struct,
                            mojo::internal::ValidationError::ILLEGAL_HANDLE);
  }

  {
    SCOPED_TRACE("Uninitialized required Handle in an Array");
    HandleStruct handle_struct;
    handle_struct.array_h = Array<mojo::ScopedMessagePipeHandle>::New(1);
    // This won't die (i.e., we don't need to EXPECT_DEATH) because the handle
    // is invalid, so should be serializable.  Instead, we live with a
    // serialization error for an invalid handle.
    // TODO(vardhan): This should be
    // mojo::internal::ValidationError::UNEXPECTED_INVALID_HANDLE after handles
    // are encoded inline with serialization.
    SerializeAndDeserialize(&handle_struct,
                            mojo::internal::ValidationError::ILLEGAL_HANDLE);
  }

  // We shouldn't be able to serialize a valid handle.
  {
    SCOPED_TRACE("Serializing a Handle");
    HandleStruct handle_struct;
    handle_struct.h = MessagePipe().handle0.Pass();
    handle_struct.array_h = Array<mojo::ScopedMessagePipeHandle>::New(0);
    EXPECT_DEATH_IF_SUPPORTED({
      SerializeAndDeserialize(&handle_struct,
                              mojo::internal::ValidationError::NONE);
    }, "does not support handles");
  }
}

// We should be able to Serialize/Deserialize a struct that has a nullable
// handle which is null.
TEST_F(StructSerializationAPITest, NullableHandleSerialization) {
  NullableHandleStruct handle_struct;
  handle_struct.data = 16;
  SerializeAndDeserialize(&handle_struct,
                          mojo::internal::ValidationError::NONE);
}

// Test that |Deserialize()| appropriately fails on validation.
TEST_F(StructSerializationAPITest, DeserializationFailure) {
  char buf[100] = {};
  EmptyStruct es;

  // Bounds checker should fail this, since buf_size is too small.
  EXPECT_FALSE(es.Deserialize(buf, 1));

  es.Serialize(buf, sizeof(buf));
  EXPECT_TRUE(es.Deserialize(buf, sizeof(buf)));

  // Invalid struct header: this should happen inside
  // EmptyStruct::Data_::Validate()).
  es.Serialize(buf, sizeof(buf));
  EmptyStruct::Data_* es_data = reinterpret_cast<EmptyStruct::Data_*>(buf);
  es_data->header_.num_bytes = 0;
  EXPECT_FALSE(es.Deserialize(buf, sizeof(buf)));
}

TEST_F(StructSerializationAPITest, DeserializationWithoutValidation) {
  const int32_t kMagic = 456;
  char buf[100] = {};
  ContainsOther::Data_* co_data = reinterpret_cast<ContainsOther::Data_*>(buf);
  ContainsOther co;

  // Success case.
  co.other = kMagic;
  EXPECT_TRUE(co.Serialize(buf, sizeof(buf)));
  co.other = 0;
  co.DeserializeWithoutValidation(buf);
  EXPECT_EQ(kMagic, co.other);

  // Invalid struct header, but will pass (i.e., won't crash) anyway because we
  // don't Validate.
  co_data->header_.num_bytes = 0u;
  co.DeserializeWithoutValidation(buf);
  EXPECT_EQ(kMagic, co_data->other);
  EXPECT_EQ(0u, co_data->header_.num_bytes);
}

}  // namespace
}  // namespace test
}  // namespace mojo
