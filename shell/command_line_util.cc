// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/command_line_util.h"

#include <functional>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/context.h"
#include "shell/switches.h"

namespace shell {

namespace {
GURL GetAppURLAndSetArgs(const std::string& app_url_and_args,
                         Context* context) {
  std::vector<std::string> args;
  GURL app_url = GetAppURLAndArgs(context, app_url_and_args, &args);

  if (!args.empty())
    context->application_manager()->SetArgsForURL(args, app_url);
  return app_url;
}
}  // namespace

bool ParseArgsFor(const std::string& arg, std::string* value) {
  const std::string kArgsForSwitches[] = {
      "-" + std::string(switches::kArgsFor) + "=",
      "--" + std::string(switches::kArgsFor) + "=",
  };
  for (size_t i = 0; i < arraysize(kArgsForSwitches); i++) {
    const std::string& argsfor_switch = kArgsForSwitches[i];
    if (arg.compare(0, argsfor_switch.size(), argsfor_switch) == 0) {
      *value = arg.substr(argsfor_switch.size(), std::string::npos);
      return true;
    }
  }
  return false;
}

GURL GetAppURLAndArgs(Context* context,
                      const std::string& app_url_and_args,
                      std::vector<std::string>* args) {
  // SplitString() returns empty strings for extra delimeter characters (' ').
  base::StringTokenizer tokenizer =
      base::StringTokenizer(app_url_and_args, " ");
  tokenizer.set_quote_chars("'\"");
  while (tokenizer.GetNext()) {
    args->push_back(tokenizer.token());
  }
  args->erase(std::remove_if(args->begin(), args->end(),
                             [](const std::string& a) { return a.empty(); }),
              args->end());

  if (args->empty())
    return GURL();
  GURL app_url = context->ResolveCommandLineURL((*args)[0]);
  if (!app_url.is_valid()) {
    LOG(ERROR) << "Error: invalid URL: " << (*args)[0];
    return app_url;
  }
  args->erase(args->begin());
  return app_url;
}

void ApplyApplicationArgs(Context* context, const std::string& args) {
  std::string args_for_value;
  if (ParseArgsFor(args, &args_for_value))
    GetAppURLAndSetArgs(args_for_value, context);
}

void RunCommandLineApps(Context* context) {
  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  for (const auto& arg : command_line.GetArgs()) {
    GURL url = GetAppURLAndSetArgs(arg, context);
    if (!url.is_valid())
      return;
    context->Run(url);
  }
}

}  // namespace shell
