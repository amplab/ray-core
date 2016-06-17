// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <deque>
#include <fstream>
#include <string>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "mojo/services/prediction/interfaces/prediction.mojom.h"
#include "mojo/tools/embed/data.h"
#include "services/prediction/dictionary_service.h"
#include "services/prediction/input_info.h"
#include "services/prediction/kDictFile.h"
#include "services/prediction/key_set.h"
#include "services/prediction/proximity_info_factory.h"
#include "third_party/android_prediction/suggest/core/dictionary/dictionary.h"
#include "third_party/android_prediction/suggest/core/result/suggestion_results.h"
#include "third_party/android_prediction/suggest/core/session/dic_traverse_session.h"
#include "third_party/android_prediction/suggest/core/session/prev_words_info.h"
#include "third_party/android_prediction/suggest/core/suggest_options.h"
#include "third_party/android_prediction/suggest/policyimpl/dictionary/structure/dictionary_structure_with_buffer_policy_factory.h"

namespace prediction {

DictionaryService::DictionaryService() : max_suggestion_size_(50) {
}

DictionaryService::~DictionaryService() {
}

void DictionaryService::CreatDictFromEmbeddedDataIfNotExist(
    const std::string path) {
  if (std::ifstream(path.c_str()))
    return;
  std::ofstream dic_file(path.c_str(),
                         std::ofstream::out | std::ofstream::binary);
  dic_file.write(prediction::kDictFile.data, prediction::kDictFile.size);
  dic_file.close();
}

latinime::Dictionary* const DictionaryService::OpenDictionary(
    const std::string path,
    const int start_offset,
    const int size,
    const bool is_updatable) {
  latinime::DictionaryStructureWithBufferPolicy::StructurePolicyPtr
      dictionary_structure_with_buffer_policy(
          latinime::DictionaryStructureWithBufferPolicyFactory::
              newPolicyForExistingDictFile(path.c_str(), start_offset, size,
                                           is_updatable));
  if (!dictionary_structure_with_buffer_policy) {
    return nullptr;
  }

  latinime::Dictionary* const dictionary = new latinime::Dictionary(
      std::move(dictionary_structure_with_buffer_policy));
  return dictionary;
}

mojo::Array<mojo::String> DictionaryService::GetDictionarySuggestion(
    PredictionInfoPtr prediction_info,
    latinime::ProximityInfo* proximity_info) {
  mojo::Array<mojo::String> suggestion_words =
      mojo::Array<mojo::String>::New(0);

  // dictionary
  base::FilePath dir_temp;
  PathService::Get(base::DIR_TEMP, &dir_temp);
  std::string path = dir_temp.value() + "/main_en.dict";
  if (!default_dictionary_) {
    CreatDictFromEmbeddedDataIfNotExist(path);
    default_dictionary_ = scoped_ptr<latinime::Dictionary>(
        OpenDictionary(path, 0, prediction::kDictFile.size, false));
    if (!default_dictionary_) {
      return suggestion_words;
    }
  }

  // dic_traverse_session
  if (!default_session_) {
    default_session_ = scoped_ptr<latinime::DicTraverseSession>(
        reinterpret_cast<latinime::DicTraverseSession*>(
            latinime::DicTraverseSession::getSessionInstance(
                "en", prediction::kDictFile.size)));
    latinime::PrevWordsInfo empty_prev_words;
    default_session_->init(default_dictionary_.get(), &empty_prev_words, 0);
  }

  // current word
  int input_size = std::min(
      static_cast<int>(prediction_info->current_word.size()), MAX_WORD_LENGTH);
  InputInfo input_info(prediction_info->current_word, input_size);
  input_size = input_info.GetRealSize();

  // previous words
  latinime::PrevWordsInfo prev_words_info =
      ProcessPrevWord(prediction_info->previous_words);

  // suggestion options
  // is_gesture, use_full_edit_distance,
  // block_offensive_words, space_aware gesture_enabled
  int options[] = {0, 0, 0, 0};
  latinime::SuggestOptions suggest_options(options, arraysize(options));

  latinime::SuggestionResults suggestion_results(max_suggestion_size_);
  if (input_size > 0) {
    default_dictionary_->getSuggestions(
        proximity_info, default_session_.get(), input_info.GetXCoordinates(),
        input_info.GetYCoordinates(), input_info.GetTimes(),
        input_info.GetPointerIds(), input_info.GetCodepoints(), input_size,
        &prev_words_info, &suggest_options, -1.0f, &suggestion_results);
  } else {
    default_dictionary_->getPredictions(&prev_words_info, &suggestion_results);
  }

  // process suggestion results
  std::deque<std::string> suggestion_words_reverse;
  char cur_beginning;
  std::string lo_cur;
  std::string up_cur;
  if (input_size > 0) {
    cur_beginning = prediction_info->current_word[0];
    std::string cur_rest =
        std::string(prediction_info->current_word.data())
            .substr(1, prediction_info->current_word.size() - 1);
    lo_cur = std::string(1, (char)tolower(cur_beginning)) + cur_rest;
    up_cur = std::string(1, (char)toupper(cur_beginning)) + cur_rest;
  }
  while (!suggestion_results.mSuggestedWords.empty()) {
    const latinime::SuggestedWord& suggested_word =
        suggestion_results.mSuggestedWords.top();
    base::string16 word;
    for (int i = 0; i < suggested_word.getCodePointCount(); i++) {
      base::char16 code_point = suggested_word.getCodePoint()[i];
      word.push_back(code_point);
    }
    std::string word_string = base::UTF16ToUTF8(word);
    if (word_string.compare(lo_cur) != 0 && word_string.compare(up_cur) != 0) {
      if (input_size > 0 && isupper(cur_beginning)) {
        word_string[0] = toupper(word_string[0]);
      }
      suggestion_words_reverse.push_front(word_string);
    }
    suggestion_results.mSuggestedWords.pop();
  }

  // remove dups within suggestion words
  for (size_t i = 0; i < suggestion_words_reverse.size(); i++) {
    for (size_t j = i + 1; j < suggestion_words_reverse.size(); j++) {
      if (suggestion_words_reverse[i].compare(suggestion_words_reverse[j]) ==
          0) {
        suggestion_words_reverse.erase(suggestion_words_reverse.begin() + j);
        j--;
      }
    }
  }

  for (std::deque<std::string>::iterator it = suggestion_words_reverse.begin();
       it != suggestion_words_reverse.end(); ++it) {
    suggestion_words.push_back(mojo::String(*it));
  }

  return suggestion_words;
}

// modified from Android JniDataUtils::constructPrevWordsInfo
latinime::PrevWordsInfo DictionaryService::ProcessPrevWord(
    mojo::Array<PrevWordInfoPtr>& prev_words) {
  int prev_word_codepoints[MAX_PREV_WORD_COUNT_FOR_N_GRAM][MAX_WORD_LENGTH] = {
      {0}};
  int prev_word_codepoint_count[MAX_PREV_WORD_COUNT_FOR_N_GRAM] = {0};
  bool are_beginning_of_sentence[MAX_PREV_WORD_COUNT_FOR_N_GRAM] = {false};
  int prevwords_count = std::min(
      prev_words.size(), static_cast<size_t>(MAX_PREV_WORD_COUNT_FOR_N_GRAM));
  for (int i = 0; i < prevwords_count; ++i) {
    prev_word_codepoint_count[i] = 0;
    are_beginning_of_sentence[i] = false;
    int prev_word_size = prev_words[i]->word.size();
    if (prev_word_size > MAX_WORD_LENGTH) {
      continue;
    }
    for (int j = 0; j < prev_word_size; j++) {
      prev_word_codepoints[i][j] = (int)((prev_words[i])->word)[j];
    }
    prev_word_codepoint_count[i] = prev_word_size;
    are_beginning_of_sentence[i] = prev_words[i]->is_beginning_of_sentence;
  }
  latinime::PrevWordsInfo prev_words_info =
      latinime::PrevWordsInfo(prev_word_codepoints, prev_word_codepoint_count,
                              are_beginning_of_sentence, prevwords_count);
  return prev_words_info;
}

}  // namespace prediction
