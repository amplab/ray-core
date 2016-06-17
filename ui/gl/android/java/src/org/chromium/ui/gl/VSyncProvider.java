// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.gl;

import android.view.Choreographer;
import android.view.Choreographer.FrameCallback;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Java implementation of ui/gfx/vsync_provider.h that uses the {@link Choreographer} to compute the
 * base time and the refresh interval.
 */
@JNINamespace("gfx")
public class VSyncProvider implements FrameCallback {
    private static final long MAX_DRIFT_PERCENT = 5;
    // A refresh period of more than 1s is considered bogus.
    private static final long MAX_VSYNC_REFRESH_NANO = 1000000000L;
    // A refresh period bigger than 1000Hz is considered bogus.
    private static final long MIN_VSYNC_REFRESH_NANO = 1000000L;
    private final Choreographer mChoreographer;
    private boolean mMonitoring;
    private long mNativeAndroidVSyncProvider;
    private RollingMedianComputer mVSyncRefreshNanoComputer;
    private long mLastSentVSyncRefreshNano;
    private long mLastSentTimeSynchronizationNano;
    private long mLastTimeNano;

    @CalledByNative
    public static VSyncProvider create() {
        return new VSyncProvider();
    }

    private VSyncProvider() {
        mChoreographer = Choreographer.getInstance();
    }

    /**
     * Returns the minimal error on |time2| in percent considering that |time1| and |time2| are
     * supposed to be to event separated by a number of |period|. This is defined as the minimal
     * time error between |time1| and |time2| such that |time2 - time1| is a integer number of
     * period, divided by |period|
     */
    private static long computeSynchronizationDriftPercent(long time1, long time2, long period) {
        long remainder = (time2 - time1) % period;
        long minError = Math.min(remainder, period - remainder);
        return 100 * minError / period;
    }

    private static long computeRefreshDriftPercent(long refresh1, long refresh2) {
        return 100 * Math.abs(refresh2 - refresh1) / refresh2;
    }

    @Override
    public void doFrame(final long timeNano) {
        if (mNativeAndroidVSyncProvider == 0) {
            mMonitoring = false;
            return;
        }

        if (timeNano <= mLastTimeNano) {
            // Spurious callback from the system.
            return;
        }

        mChoreographer.postFrameCallback(this);
        if (mLastTimeNano != 0) {
            long currentRefreshNano = timeNano - mLastTimeNano;
            if (currentRefreshNano > MIN_VSYNC_REFRESH_NANO
                    && currentRefreshNano < MAX_VSYNC_REFRESH_NANO) {
                mVSyncRefreshNanoComputer.add(currentRefreshNano);
                if (mLastSentTimeSynchronizationNano == 0
                        || computeSynchronizationDriftPercent(timeNano,
                                   mLastSentTimeSynchronizationNano, mLastSentVSyncRefreshNano)
                                > MAX_DRIFT_PERCENT
                        || computeRefreshDriftPercent(currentRefreshNano, mLastSentVSyncRefreshNano)
                                > MAX_DRIFT_PERCENT) {
                    mLastSentTimeSynchronizationNano = timeNano;
                    mLastSentVSyncRefreshNano = mVSyncRefreshNanoComputer.getMedian();
                    nativeOnSyncChanged(mNativeAndroidVSyncProvider, timeNano / 1000,
                            mLastSentVSyncRefreshNano / 1000);
                }
            }
        }
        mLastTimeNano = timeNano;
    }

    @CalledByNative
    private void startMonitoring(long nativeAndroidVsyncProvider) {
        mLastSentVSyncRefreshNano = 0;
        mLastSentTimeSynchronizationNano = 0;
        mVSyncRefreshNanoComputer = new RollingMedianComputer();
        mNativeAndroidVSyncProvider = nativeAndroidVsyncProvider;
        if (!mMonitoring) {
            mChoreographer.postFrameCallback(this);
            mMonitoring = true;
        }
    }

    @CalledByNative
    private void stopMonitoring() {
        mNativeAndroidVSyncProvider = 0;
    }

    private native void nativeOnSyncChanged(long nativeAndroidVSyncProvider, long time, long delay);
}
