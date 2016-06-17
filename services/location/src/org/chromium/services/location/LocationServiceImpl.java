// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.location;

import android.location.Location;
import android.os.Looper;

import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.location.LocationListener;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationServices;

import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.RunLoop;
import org.chromium.mojom.mojo.LocationService;

/**
 * Implementation of org.chromium.mojom.mojo.LocationService.
 */
class LocationServiceImpl implements LocationService, LocationListener {
    private final GoogleApiClient mGoogleApiClient;
    private final Looper mLooper;
    private final RunLoop mMessageLoop;
    private LocationService.GetNextLocationResponse mCallback;
    private boolean mIsInitialCall;
    private static final int LOCATION_UPDATE_INTERVAL_MILLIS = 10000;

    public LocationServiceImpl(GoogleApiClient client, Looper looper, RunLoop runLoop) {
        mGoogleApiClient = client;
        mLooper = looper;
        mMessageLoop = runLoop;
        mIsInitialCall = true;
    }

    /**
     * Ful-fills a pending callback if available on a location change update. This must be called on
     * the mojo message loop thread.
     *
     * @param location New location update.
     */
    private void callbackWithResponse(Location location) {
        if (mCallback != null) {
            mCallback.call(LocationUtil.toMojoLocation(location));
            mCallback = null;
        }
    }

    /**
     * Called when a new location update is available. This is the only method called back on the
     * looper thread.
     *
     * @see com.google.android.gms.location.LocationListener#onLocationChanged(Location)
     */
    @Override
    public void onLocationChanged(final Location location) {
        mMessageLoop.postDelayedTask(new Runnable() {
            @Override
            public void run() {
                callbackWithResponse(location);
            }
        }, 0);
    }

    /**
     * When |getNextLocation| is called for the first time, we return the last location. For
     * subsequent calls, we register for updates at the requested priority.
     *
     * @see org.chromium.mojom.mojo.LocationService#getNextLocation(int,
     *      LocationService.GetNextLocationResponse)
     */
    @Override
    public void getNextLocation(int priority, LocationService.GetNextLocationResponse callback) {
        // Returns null immediately if there is a pending callback.
        if (mCallback != null) {
            callback.call(null);
            return;
        }

        if (mIsInitialCall) {
            mIsInitialCall = false;
            Location lastLocation =
                    LocationServices.FusedLocationApi.getLastLocation(mGoogleApiClient);

            if (lastLocation != null) {
                callback.call(LocationUtil.toMojoLocation(lastLocation));
                return;
            }
        }

        mCallback = callback;
        final LocationRequest locationRequest = new LocationRequest();
        locationRequest.setInterval(LOCATION_UPDATE_INTERVAL_MILLIS);
        locationRequest.setPriority(LocationUtil.toAndroidLocationPriority(priority));
        LocationServices.FusedLocationApi.requestLocationUpdates(
                mGoogleApiClient, locationRequest, this, mLooper);
    }

    /**
     * When connection is closed, we stop receiving location updates.
     *
     * @see org.chromium.mojo.bindings.Interface#close()
     */
    @Override
    public void close() {
        LocationServices.FusedLocationApi.removeLocationUpdates(mGoogleApiClient, this);
    }

    /**
     * @see org.chromium.mojo.bindings.Interface#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {}
}
