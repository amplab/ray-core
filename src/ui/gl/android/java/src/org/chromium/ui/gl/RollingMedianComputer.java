// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.gl;

import java.util.Arrays;

/**
 * Compute a rolling median over a fixed window of an unbounded stream.
 */
public class RollingMedianComputer {
    private final long mValues[] = {0, 0, 0, Long.MAX_VALUE, Long.MAX_VALUE};
    private int mNextIndex = 0;
    private final long mSortedArray[] = new long[mValues.length];
    private boolean mSorted = false;

    public void add(long element) {
        mValues[mNextIndex] = element;
        mNextIndex = (mNextIndex + 1) % mValues.length;
        mSorted = false;
    }

    public long getMedian() {
        if (!mSorted) {
            System.arraycopy(mValues, 0, mSortedArray, 0, mValues.length);
            Arrays.sort(mSortedArray);
            mSorted = true;
        }
        return mSortedArray[mValues.length / 2];
    }
}
