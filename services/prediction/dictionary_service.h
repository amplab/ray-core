// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREDICTION_DICTIONARY_SERVICE_H_
#define SERVICES_PREDICTION_DICTIONARY_SERVICE_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/services/prediction/interfaces/prediction.mojom.h"
#include "services/prediction/proximity_info_factory.h"

namespace latinime {

class Dictionary;
class DicTraverseSession;
class PrevWordsInfo;
}  // namespace latinime

namespace prediction {

class DictionaryService {
 public:
  DictionaryService();
  ~DictionaryService();

  mojo::Array<mojo::String> GetDictionarySuggestion(
      PredictionInfoPtr prediction_info,
      latinime::ProximityInfo* proximity_info);

 private:
  void CreatDictFromEmbeddedDataIfNotExist(const std::string path);

  latinime::Dictionary* const OpenDictionary(const std::string path,
                                             const int start_offset,
                                             const int size,
                                             const bool is_updatable);

  latinime::PrevWordsInfo ProcessPrevWord(
      mojo::Array<PrevWordInfoPtr>& prev_words);

  int max_suggestion_size_;
  scoped_ptr<latinime::Dictionary> default_dictionary_;
  scoped_ptr<latinime::DicTraverseSession> default_session_;

  DISALLOW_COPY_AND_ASSIGN(DictionaryService);
};

}  // namespace prediction

#endif  // SERVICES_PREDICTION_DICTIONARY_SERVICE_H_
