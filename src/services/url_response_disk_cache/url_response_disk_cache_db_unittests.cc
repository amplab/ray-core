// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/scoped_temp_dir.h"
#include "base/threading/platform_thread.h"
#include "services/url_response_disk_cache/url_response_disk_cache_db.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {

namespace {

class URLResponseDiskCacheDBTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(tmp_dir_.CreateUniqueTempDir());
    Open();
  }

  void Open() {
    db_ = nullptr;
    base::FilePath db_path = tmp_dir_.path().Append("db");
    db_ = new URLResponseDiskCacheDB(db_path);
  }

  CacheEntryPtr NewEntry() {
    CacheEntryPtr entry = CacheEntry::New();
    entry->response = URLResponse::New();
    entry->entry_directory = "/cache";
    entry->response_body_path = "/cache/content";
    return entry;
  }

  base::ScopedTempDir tmp_dir_;
  scoped_refptr<URLResponseDiskCacheDB> db_;
};

TEST_F(URLResponseDiskCacheDBTest, Create) {}

TEST_F(URLResponseDiskCacheDBTest, Version) {
  EXPECT_EQ(0lu, db_->GetVersion());
  db_->SetVersion(15);
  EXPECT_EQ(15lu, db_->GetVersion());
}

TEST_F(URLResponseDiskCacheDBTest, Persist) {
  db_->SetVersion(15);
  EXPECT_EQ(15lu, db_->GetVersion());
  Open();
  EXPECT_EQ(15lu, db_->GetVersion());
}

TEST_F(URLResponseDiskCacheDBTest, Entry) {
  std::string origin = "origin";
  std::string url = "url";
  db_->PutNew(origin, url, NewEntry());
  CacheKeyPtr key;
  CacheEntryPtr entry = db_->GetNewest(origin, url, &key);
  EXPECT_TRUE(key);
  EXPECT_TRUE(entry);
  Open();
  CacheKeyPtr key2;
  entry = db_->GetNewest(origin, url, &key2);
  EXPECT_TRUE(entry);
  EXPECT_TRUE(key.Equals(key2));

  std::string new_entry_directory = "/newcache/";
  entry->entry_directory = new_entry_directory;
  db_->Put(key.Clone(), entry.Pass());
  entry = db_->GetNewest(origin, url, nullptr);
  EXPECT_TRUE(entry);
  EXPECT_EQ(new_entry_directory, entry->entry_directory);
  entry = db_->Get(key.Pass());
  EXPECT_TRUE(entry);
  EXPECT_EQ(new_entry_directory, entry->entry_directory);
}

TEST_F(URLResponseDiskCacheDBTest, Newest) {
  std::string origin = "origin";
  std::string url = "url";
  std::string new_entry_directory = "/newcache/";
  db_->PutNew(origin, url, NewEntry());
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(5));
  CacheEntryPtr entry = NewEntry();
  entry->entry_directory = new_entry_directory;
  db_->PutNew(origin, url, entry.Pass());
  entry = db_->GetNewest(origin, url, nullptr);
  EXPECT_TRUE(entry);
  EXPECT_EQ(new_entry_directory, entry->entry_directory);
}

TEST_F(URLResponseDiskCacheDBTest, Iterator) {
  std::string origin = "origin";
  std::string url1 = "a";
  std::string url2 = "b";
  std::string url3 = "c";
  db_->PutNew(origin, url2, NewEntry());
  CacheEntryPtr entry = db_->GetNewest(origin, url2, nullptr);
  EXPECT_TRUE(entry);
  db_->PutNew(origin, url1, NewEntry());
  db_->PutNew(origin, url3, NewEntry());
  entry = CacheEntry::New();
  scoped_ptr<URLResponseDiskCacheDB::Iterator> iterator = db_->GetIterator();
  EXPECT_TRUE(iterator->HasNext());
  CacheKeyPtr key;
  iterator->GetNext(&key, &entry);
  EXPECT_TRUE(iterator->HasNext());
  EXPECT_TRUE(key);
  EXPECT_EQ(url1, key->url);
  EXPECT_TRUE(entry);
  iterator->GetNext(&key, &entry);
  EXPECT_TRUE(iterator->HasNext());
  EXPECT_TRUE(key);
  EXPECT_EQ(url2, key->url);
  EXPECT_TRUE(entry);
  iterator->GetNext(&key, &entry);
  EXPECT_FALSE(iterator->HasNext());
  EXPECT_TRUE(key);
  EXPECT_EQ(url3, key->url);
  EXPECT_TRUE(entry);
}

TEST_F(URLResponseDiskCacheDBTest, Delete) {
  std::string origin = "origin";
  std::string url = "url";
  db_->PutNew(origin, url, NewEntry());
  CacheEntryPtr entry = db_->GetNewest(origin, url, nullptr);
  EXPECT_TRUE(entry);
  entry = CacheEntry::New();
  scoped_ptr<URLResponseDiskCacheDB::Iterator> iterator = db_->GetIterator();
  EXPECT_TRUE(iterator->HasNext());
  CacheKeyPtr key;
  iterator->GetNext(&key, &entry);
  EXPECT_FALSE(iterator->HasNext());
  EXPECT_TRUE(key);
  EXPECT_TRUE(entry);
  db_->Delete(key.Pass());
  entry = db_->GetNewest(origin, url, nullptr);
  EXPECT_FALSE(entry);
}

TEST_F(URLResponseDiskCacheDBTest, IteratorFrozen) {
  std::string origin = "origin";
  std::string url = "url";
  db_->PutNew(origin, url, NewEntry());
  scoped_ptr<URLResponseDiskCacheDB::Iterator> iterator = db_->GetIterator();
  std::string url2 = "url2";
  db_->PutNew(origin, url2, NewEntry());

  EXPECT_TRUE(iterator->HasNext());
  iterator->GetNext(nullptr, nullptr);
  EXPECT_FALSE(iterator->HasNext());
}

}  // namespace

}  // namespace mojo
