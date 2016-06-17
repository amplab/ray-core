// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/graph/scene_label.h"

#include <cinttypes>

#include "base/strings/stringprintf.h"

namespace compositor {

SceneLabel::SceneLabel(uint32_t token, const std::string& label)
    : token_(token), label_(label) {}

SceneLabel::SceneLabel(const SceneLabel& other)
    : token_(other.token_), label_(other.label_) {}

SceneLabel::~SceneLabel() {}

std::string SceneLabel::FormattedLabel() const {
  return label_.empty()
             ? base::StringPrintf("<S%d>", token_)
             : base::StringPrintf("<S%d:%s>", token_, label_.c_str());
}

std::string SceneLabel::FormattedLabelForVersion(
    uint32_t version,
    int64_t presentation_time) const {
  return label_.empty()
             ? base::StringPrintf("<S%d/v%d@%" PRId64 ">", token_, version,
                                  presentation_time)
             : base::StringPrintf("<S%d:%s/v%d@%" PRId64 ">", token_,
                                  label_.c_str(), version, presentation_time);
}

std::string SceneLabel::FormattedLabelForNode(uint32_t version,
                                              int64_t presentation_time,
                                              uint32_t node_id) const {
  return label_.empty()
             ? base::StringPrintf("<S%d/v%d@%" PRId64 ">[#%d]", token_, version,
                                  presentation_time, node_id)
             : base::StringPrintf("<S%d:%s/v%d@%" PRId64 ">[#%d]", token_,
                                  label_.c_str(), version, presentation_time,
                                  node_id);
}

}  // namespace compositor
