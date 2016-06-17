/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LATINIME_DIC_TRAVERSE_SESSION_H
#define LATINIME_DIC_TRAVERSE_SESSION_H

#include <vector>

#include "third_party/android_prediction/defines.h"
#include "third_party/android_prediction/suggest/core/dicnode/dic_nodes_cache.h"
#include "third_party/android_prediction/suggest/core/dictionary/multi_bigram_map.h"
#include "third_party/android_prediction/suggest/core/layout/proximity_info_state.h"

namespace latinime {

class Dictionary;
class DictionaryStructureWithBufferPolicy;
class PrevWordsInfo;
class ProximityInfo;
class SuggestOptions;

class DicTraverseSession {
 public:

    // A factory method for DicTraverseSession
    static AK_FORCE_INLINE void *getSessionInstance(std::string localeStr,
            long dictSize) {
        // To deal with the trade-off between accuracy and memory space, large cache is used for
        // dictionaries larger that the threshold
        return new DicTraverseSession(localeStr,
                dictSize >= DICTIONARY_SIZE_THRESHOLD_TO_USE_LARGE_CACHE_FOR_SUGGESTION);
    }

    static AK_FORCE_INLINE void releaseSessionInstance(DicTraverseSession *traverseSession) {
        delete traverseSession;
    }

    AK_FORCE_INLINE DicTraverseSession(std::string localeStr, bool usesLargeCache)
            : mProximityInfo(nullptr), mDictionary(nullptr), mSuggestOptions(nullptr),
              mDicNodesCache(usesLargeCache), mMultiBigramMap(), mInputSize(0), mMaxPointerCount(1),
              mMultiWordCostMultiplier(1.0f) {
        // NOTE: mProximityInfoStates is an array of instances.
        // No need to initialize it explicitly here.
        for (size_t i = 0; i < NELEMS(mPrevWordsPtNodePos); ++i) {
            mPrevWordsPtNodePos[i] = NOT_A_DICT_POS;
        }
    }

    // Non virtual inline destructor -- never inherit this class
    AK_FORCE_INLINE ~DicTraverseSession() {}

    void init(const Dictionary *dictionary, const PrevWordsInfo *const prevWordsInfo,
            const SuggestOptions *const suggestOptions);
    // TODO: Remove and merge into init
    void setupForGetSuggestions(const ProximityInfo *pInfo, const int *inputCodePoints,
            const int inputSize, const int *const inputXs, const int *const inputYs,
            const int *const times, const int *const pointerIds, const float maxSpatialDistance,
            const int maxPointerCount);
    void resetCache(const int thresholdForNextActiveDicNodes, const int maxWords);

    const DictionaryStructureWithBufferPolicy *getDictionaryStructurePolicy() const;

    //--------------------
    // getters and setters
    //--------------------
    const ProximityInfo *getProximityInfo() const { return mProximityInfo; }
    const SuggestOptions *getSuggestOptions() const { return mSuggestOptions; }
    const int *getPrevWordsPtNodePos() const { return mPrevWordsPtNodePos; }
    DicNodesCache *getDicTraverseCache() { return &mDicNodesCache; }
    MultiBigramMap *getMultiBigramMap() { return &mMultiBigramMap; }
    const ProximityInfoState *getProximityInfoState(int id) const {
        return &mProximityInfoStates[id];
    }
    int getInputSize() const { return mInputSize; }

    bool isOnlyOnePointerUsed(int *pointerId) const {
        // Not in the dictionary word
        int usedPointerCount = 0;
        int usedPointerId = 0;
        for (int i = 0; i < mMaxPointerCount; ++i) {
            if (mProximityInfoStates[i].isUsed()) {
                ++usedPointerCount;
                usedPointerId = i;
            }
        }
        if (usedPointerCount != 1) {
            return false;
        }
        if (pointerId) {
            *pointerId = usedPointerId;
        }
        return true;
    }

    ProximityType getProximityTypeG(const DicNode *const dicNode, const int childCodePoint) const {
        ProximityType proximityType = UNRELATED_CHAR;
        for (int i = 0; i < MAX_POINTER_COUNT_G; ++i) {
            if (!mProximityInfoStates[i].isUsed()) {
                continue;
            }
            const int pointerId = dicNode->getInputIndex(i);
            proximityType = mProximityInfoStates[i].getProximityTypeG(pointerId, childCodePoint);
            ASSERT(proximityType == UNRELATED_CHAR || proximityType == MATCH_CHAR);
            // TODO: Make this more generic
            // Currently we assume there are only two types here -- UNRELATED_CHAR
            // and MATCH_CHAR
            if (proximityType != UNRELATED_CHAR) {
                return proximityType;
            }
        }
        return proximityType;
    }

    AK_FORCE_INLINE bool isCacheBorderForTyping(const int inputSize) const {
        return mDicNodesCache.isCacheBorderForTyping(inputSize);
    }

    /**
     * Returns whether or not it is possible to continue suggestion from the previous search.
     */
    // TODO: Remove. No need to check once the session is fully implemented.
    bool isContinuousSuggestionPossible() const {
        if (!mDicNodesCache.hasCachedDicNodesForContinuousSuggestion()) {
            return false;
        }
        ASSERT(mMaxPointerCount <= MAX_POINTER_COUNT_G);
        for (int i = 0; i < mMaxPointerCount; ++i) {
            const ProximityInfoState *const pInfoState = getProximityInfoState(i);
            // If a proximity info state is not continuous suggestion possible,
            // do not continue searching.
            if (pInfoState->isUsed() && !pInfoState->isContinuousSuggestionPossible()) {
                return false;
            }
        }
        return true;
    }

    bool isTouchPositionCorrectionEnabled() const {
        return mProximityInfoStates[0].touchPositionCorrectionEnabled();
    }

    float getMultiWordCostMultiplier() const {
        return mMultiWordCostMultiplier;
    }

 private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(DicTraverseSession);
    // threshold to start caching
    static const int CACHE_START_INPUT_LENGTH_THRESHOLD;
    static const int DICTIONARY_SIZE_THRESHOLD_TO_USE_LARGE_CACHE_FOR_SUGGESTION;
    void initializeProximityInfoStates(const int *const inputCodePoints, const int *const inputXs,
            const int *const inputYs, const int *const times, const int *const pointerIds,
            const int inputSize, const float maxSpatialDistance, const int maxPointerCount);

    int mPrevWordsPtNodePos[MAX_PREV_WORD_COUNT_FOR_N_GRAM];
    const ProximityInfo *mProximityInfo;
    const Dictionary *mDictionary;
    const SuggestOptions *mSuggestOptions;

    DicNodesCache mDicNodesCache;
    // Temporary cache for bigram frequencies
    MultiBigramMap mMultiBigramMap;
    ProximityInfoState mProximityInfoStates[MAX_POINTER_COUNT_G];

    int mInputSize;
    int mMaxPointerCount;

    /////////////////////////////////
    // Configuration per dictionary
    float mMultiWordCostMultiplier;

};
} // namespace latinime
#endif // LATINIME_DIC_TRAVERSE_SESSION_H
