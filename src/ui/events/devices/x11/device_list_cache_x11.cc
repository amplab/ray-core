// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/devices/x11/device_list_cache_x11.h"

#include <algorithm>

#include "base/memory/singleton.h"
#include "base/message_loop/message_loop.h"
#include "ui/events/devices/x11/device_data_manager_x11.h"

namespace ui {

DeviceListCacheX11::DeviceListCacheX11() {
}

DeviceListCacheX11::~DeviceListCacheX11() {
}

DeviceListCacheX11* DeviceListCacheX11::GetInstance() {
  return base::Singleton<DeviceListCacheX11>::get();
}

void DeviceListCacheX11::UpdateDeviceList(Display* display) {
  XDeviceList& new_x_dev_list = x_dev_list_;
  new_x_dev_list.devices.reset(
      XListInputDevices(display, &new_x_dev_list.count));

  xi_dev_list_.devices.reset();
}

const XDeviceList& DeviceListCacheX11::GetXDeviceList(Display* display) {
  XDeviceList& x_dev_list = x_dev_list_;
  // Note that the function can be called before any update has taken place.
  if (!x_dev_list.devices && !x_dev_list.count)
    x_dev_list.devices.reset(XListInputDevices(display, &x_dev_list.count));
  return x_dev_list;
}

const XIDeviceList& DeviceListCacheX11::GetXI2DeviceList(Display* display) {
  XIDeviceList& xi_dev_list = xi_dev_list_;
  if (!xi_dev_list.devices && !xi_dev_list.count) {
    xi_dev_list.devices.reset(
        XIQueryDevice(display, XIAllDevices, &xi_dev_list.count));
  }
  return xi_dev_list;
}

}  // namespace ui

