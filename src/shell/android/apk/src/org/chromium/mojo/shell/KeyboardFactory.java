// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import org.chromium.base.ApplicationStatus;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.keyboard.KeyboardServiceImpl;
import org.chromium.mojom.keyboard.KeyboardService;

/**
 * A ServiceFactoryBinder for the keyboard service.
 */
final class KeyboardFactory implements ServiceFactoryBinder<KeyboardService> {
    @Override
    public void bind(InterfaceRequest<KeyboardService> request) {
        KeyboardService.MANAGER.bind(
                new KeyboardServiceImpl(ApplicationStatus.getApplicationContext()), request);
    }

    @Override
    public String getInterfaceName() {
        return KeyboardService.MANAGER.getName();
    }
}
