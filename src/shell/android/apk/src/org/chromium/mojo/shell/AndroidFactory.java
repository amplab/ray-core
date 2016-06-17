// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import org.chromium.mojo.android.ActivityImpl;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojom.activity.Activity;

/**
 * A ServiceFactoryBinder for the android service.
 */
final class AndroidFactory implements ServiceFactoryBinder<Activity> {
    @Override
    public void bind(final InterfaceRequest<Activity> request) {
        if (ViewportActivity.getCurrent() == null) {
            request.close();
            return;
        }
        Activity.MANAGER.bind(new ActivityImpl(ViewportActivity.getCurrent()), request);
    }

    @Override
    public String getInterfaceName() {
        return Activity.MANAGER.getName();
    }
}
