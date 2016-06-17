// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_DB_H_
#define SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_DB_H_

#include "base/files/file_path.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "services/url_response_disk_cache/url_response_disk_cache_entry.mojom.h"

namespace leveldb {
class Comparator;
class DB;
class Iterator;
};

namespace mojo {

// Specialized database for the cache content.
class URLResponseDiskCacheDB
    : public base::RefCountedThreadSafe<URLResponseDiskCacheDB> {
 public:
  class Iterator {
   public:
    explicit Iterator(linked_ptr<leveldb::DB> db);
    ~Iterator();

    bool HasNext();
    void GetNext(CacheKeyPtr* key, CacheEntryPtr* entry);

   private:
    linked_ptr<leveldb::DB> db_;
    scoped_ptr<leveldb::Iterator> it_;
  };

  // Constructs the database. |db_path| is the path to the leveldb database. If
  // the path exists, the database will be opened, otherwise it will be created.
  explicit URLResponseDiskCacheDB(const base::FilePath& db_path);

  // Set and get the version of the database.
  uint64_t GetVersion();
  void SetVersion(uint64_t version);

  // Set and get an entry for a given key.
  void Put(CacheKeyPtr key, CacheEntryPtr entry);
  CacheEntryPtr Get(CacheKeyPtr key);

  // Put an entry for the given |request_origin| and |url|. Older entry for the
  // same |request_origin| and |url| will not be removed, but will be shadowed
  // by the new one.
  void PutNew(const std::string& request_origin,
              const std::string& url,
              CacheEntryPtr entry);

  // Returns the newest entry for the given |request_origin| and |url|, or null
  // if none exist. If |key| is not null and GetNewest returns an entry, |*key|
  // will contain the actual key of the entry.
  CacheEntryPtr GetNewest(const std::string& request_origin,
                          const std::string& url,
                          CacheKeyPtr* key);

  // Delete the entry for the given |key|.
  void Delete(CacheKeyPtr key);

  // Returns an iterator over all the entries in the database. For a given
  // |request_origin| and |url|, entries will be sorted from newest to oldest.
  // An iterator will not be invalidated nor any of its values will be modified
  // by further changes to the database.
  scoped_ptr<Iterator> GetIterator();

 private:
  friend class base::RefCountedThreadSafe<URLResponseDiskCacheDB>;
  ~URLResponseDiskCacheDB();

  scoped_ptr<leveldb::Comparator> comparator_;
  linked_ptr<leveldb::DB> db_;
};

}  // namespace

#endif  // SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_DB_H_
