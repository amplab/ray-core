// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.input.InputServiceImpl;
import org.chromium.mojom.input.InputService;

/**
 * A ServiceFactoryBinder for the input service.
 */
final class InputFactory implements ServiceFactoryBinder<InputService> {
    @Override
    public void bind(InterfaceRequest<InputService> request) {
        InputService.MANAGER.bind(new InputServiceImpl(), request);
    }

    @Override
    public String getInterfaceName() {
        return InputService.MANAGER.getName();
    }
}
