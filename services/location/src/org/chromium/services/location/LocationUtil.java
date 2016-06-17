// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.location;

import android.util.Log;

import com.google.android.gms.location.LocationRequest;

import org.chromium.mojom.mojo.Location;
import org.chromium.mojom.mojo.LocationService;

/**
 * Utility class with static helper methods to convert between Android and Mojo representations.
 */
class LocationUtil {
    private static String TAG = "LocationUtil";

    private LocationUtil() {}

    /**
     * Utility method to convert location update request priority from mojo representation
     * to android representation.
     *
     * @param mojoLocationPriority Mojo location update request priority.
     * @return Android location update request priority.
     */
    public static int toAndroidLocationPriority(int mojoLocationPriority) {
        switch (mojoLocationPriority) {
            case LocationService.UpdatePriority.PRIORITY_BALANCED_POWER_ACCURACY:
                return LocationRequest.PRIORITY_BALANCED_POWER_ACCURACY;
            case LocationService.UpdatePriority.PRIORITY_HIGH_ACCURACY:
                return LocationRequest.PRIORITY_HIGH_ACCURACY;
            case LocationService.UpdatePriority.PRIORITY_LOW_POWER:
                return LocationRequest.PRIORITY_LOW_POWER;
            case LocationService.UpdatePriority.PRIORITY_NO_POWER:
                return LocationRequest.PRIORITY_NO_POWER;
            default:
                Log.e(TAG, "Unkown location service priority. Assigning lowest priority");
                return LocationRequest.PRIORITY_NO_POWER;
        }
    }

    /**
     * Utility method to convert from android location representation to mojo location
     * representation.
     *
     * @param androidLocation Android location.
     * @return Mojo location.
     */
    public static Location toMojoLocation(android.location.Location androidLocation) {
        Location mojoLocation = new Location();
        mojoLocation.time = androidLocation.getTime();

        mojoLocation.hasElapsedRealTimeNanos = true;
        mojoLocation.elapsedRealTimeNanos = androidLocation.getElapsedRealtimeNanos();

        mojoLocation.latitude = androidLocation.getLatitude();
        mojoLocation.longitude = androidLocation.getLongitude();

        mojoLocation.hasAltitude = androidLocation.hasAltitude();
        mojoLocation.altitude = androidLocation.getAltitude();

        mojoLocation.hasSpeed = androidLocation.hasSpeed();
        mojoLocation.speed = androidLocation.getSpeed();

        mojoLocation.hasAccuracy = androidLocation.hasAccuracy();
        mojoLocation.accuracy = androidLocation.getAccuracy();

        return mojoLocation;
    }
}
