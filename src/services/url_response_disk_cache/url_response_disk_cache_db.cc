// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/url_response_disk_cache/url_response_disk_cache_db.h"

#include <string.h>

#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "leveldb/comparator.h"
#include "leveldb/db.h"
#include "services/url_response_disk_cache/url_response_disk_cache_entry.mojom.h"

namespace mojo {
namespace {

// List of keys for metadata. All metadata keys must start with a '\1'
// character.
const char kVersionKey[] = "\1version";

// T is a |StructPtr|.
template <typename T>
void Serialize(T input, std::string* output) {
  size_t size = input->GetSerializedSize();
  output->clear();
  output->resize(size);

  input->Serialize(&output->at(0), size);
}

template <typename T>
bool Deserialize(void* data, size_t size, T* output) {
  *output = mojo::internal::RemoveStructPtr<T>::type::New();
  return (*output)->Deserialize(data, size);
}

template <typename T>
bool Deserialize(std::string s, T* output) {
  return Deserialize(&s.at(0), s.size(), output);
}

template <typename T>
bool Deserialize(const leveldb::Slice& s, T* output) {
  return Deserialize(s.ToString(), output);
}

// Returns whether the key is for a metadata entry. Metadata entries are
// declared at the start of this file.
bool IsMetaDataKey(const leveldb::Slice& s) {
  return s.size() != 0 && s[0] == '\1';
}

class KeyComparator : public leveldb::Comparator {
 public:
  int Compare(const leveldb::Slice& s1,
              const leveldb::Slice& s2) const override {
    if (IsMetaDataKey(s1) != IsMetaDataKey(s2)) {
      if (IsMetaDataKey(s1))
        return -1;
      return 1;
    }

    if (IsMetaDataKey(s1))
      return leveldb::BytewiseComparator()->Compare(s1, s2);

    mojo::CacheKeyPtr k1, k2;
    bool result = Deserialize(s1, &k1) && Deserialize(s2, &k2);
    DCHECK(result);
    if (k1->request_origin.get() < k2->request_origin.get())
      return -1;
    if (k1->request_origin.get() > k2->request_origin.get())
      return +1;
    if (k1->url.get() < k2->url.get())
      return -1;
    if (k1->url.get() > k2->url.get())
      return +1;
    if (k1->timestamp < k2->timestamp)
      return 1;
    if (k1->timestamp > k2->timestamp)
      return -1;
    return 0;
  }

  const char* Name() const override { return "KeyComparator"; }
  void FindShortestSeparator(std::string*,
                             const leveldb::Slice&) const override {}
  void FindShortSuccessor(std::string*) const override {}
};

}  // namespace

URLResponseDiskCacheDB::Iterator::Iterator(linked_ptr<leveldb::DB> db)
    : db_(db) {
  it_.reset(db_->NewIterator(leveldb::ReadOptions()));
  it_->SeekToFirst();
}

URLResponseDiskCacheDB::Iterator::~Iterator() {}

bool URLResponseDiskCacheDB::Iterator::HasNext() {
  while (it_->Valid() && IsMetaDataKey(it_->key())) {
    it_->Next();
  }
  return it_->Valid();
}

void URLResponseDiskCacheDB::Iterator::GetNext(CacheKeyPtr* key,
                                               CacheEntryPtr* entry) {
  // Calling HasNext() to ensure metadata are skipped.
  bool has_next = HasNext();
  DCHECK(has_next);
  if (key)
    Deserialize(it_->key(), key);
  if (entry)
    Deserialize(it_->value(), entry);
  it_->Next();
}

URLResponseDiskCacheDB::URLResponseDiskCacheDB(const base::FilePath& db_path)
    : comparator_(new KeyComparator) {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  options.comparator = comparator_.get();
  leveldb::Status status = leveldb::DB::Open(options, db_path.value(), &db);
  DCHECK(status.ok()) << status.ToString();
  db_.reset(db);
}

uint64_t URLResponseDiskCacheDB::GetVersion() {
  std::string value;
  leveldb::Status status =
      db_->Get(leveldb::ReadOptions(), kVersionKey, &value);
  if (status.IsNotFound())
    return 0u;
  DCHECK(status.ok());
  uint64_t version;
  memcpy(&version, value.data(), sizeof(version));
  return version;
}

void URLResponseDiskCacheDB::SetVersion(uint64_t version) {
  leveldb::Status status = db_->Put(
      leveldb::WriteOptions(), kVersionKey,
      leveldb::Slice(reinterpret_cast<char*>(&version), sizeof(version)));
  DCHECK(status.ok());
}

void URLResponseDiskCacheDB::Put(CacheKeyPtr key, CacheEntryPtr entry) {
  std::string key_string;
  Serialize(key.Pass(), &key_string);
  std::string entry_string;
  Serialize(entry.Pass(), &entry_string);
  leveldb::Status s =
      db_->Put(leveldb::WriteOptions(), key_string, entry_string);
  DCHECK(s.ok());
}

CacheEntryPtr URLResponseDiskCacheDB::Get(CacheKeyPtr key) {
  std::string key_string;
  std::string entry_string;
  Serialize(key.Pass(), &key_string);
  leveldb::Status status =
      db_->Get(leveldb::ReadOptions(), key_string, &entry_string);
  if (status.IsNotFound())
    return CacheEntryPtr();
  DCHECK(status.ok());
  CacheEntryPtr result;
  Deserialize(entry_string, &result);
  return result;
}

void URLResponseDiskCacheDB::PutNew(const std::string& request_origin,
                                    const std::string& url,
                                    CacheEntryPtr entry) {
  CacheKeyPtr key = CacheKey::New();
  key->request_origin = request_origin;
  key->url = url;
  key->timestamp = base::Time::Now().ToInternalValue();
  Put(key.Pass(), entry.Pass());
}

CacheEntryPtr URLResponseDiskCacheDB::GetNewest(
    const std::string& request_origin,
    const std::string& url,
    CacheKeyPtr* output_key) {
  CacheKeyPtr key = CacheKey::New();
  key->request_origin = request_origin;
  key->url = url;
  key->timestamp = std::numeric_limits<int64>::max();
  std::string key_string;
  Serialize(key.Pass(), &key_string);
  scoped_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions()));
  it->Seek(key_string);
  CacheEntryPtr result;
  if (it->Valid()) {
    Deserialize(it->key(), &key);
    if (key->request_origin == request_origin && key->url == url) {
      Deserialize(it->value(), &result);
      if (output_key) {
        *output_key = key.Pass();
      }
    }
  }
  return result;
}

void URLResponseDiskCacheDB::Delete(CacheKeyPtr key) {
  std::string key_string;
  Serialize(key.Pass(), &key_string);
  leveldb::Status s = db_->Delete(leveldb::WriteOptions(), key_string);
  DCHECK(s.ok());
}

scoped_ptr<URLResponseDiskCacheDB::Iterator>
URLResponseDiskCacheDB::GetIterator() {
  return make_scoped_ptr(new Iterator(db_));
}

URLResponseDiskCacheDB::~URLResponseDiskCacheDB() {}

}  // namespace mojo
