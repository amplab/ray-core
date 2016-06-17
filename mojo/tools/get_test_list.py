#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Central list of tests to run (as appropriate for a given config). Add tests
to run by modifying this file.

Note that this file is both imported (by mojob.py) and run directly (via a
recipe)."""


import argparse
import json
import os
import sys

from mopy.config import Config
from mopy.paths import Paths

# Url of the performance dashboard server to upload results of perftests to.
_PERFORMANCE_DASHBOARD_URL = "https://chromeperf.appspot.com"


def GetTestList(config, verbose_count=0):
  """Gets the list of tests to run for the given config. The test list (which is
  returned) is just a list of dictionaries, each dictionary having two required
  fields:
    {
      "name": "Short name",
      "command": ["python", "test_runner.py", "--some", "args"]
    }
  """

  types_to_run = set(config.test_types)

  # See above for a description of the test list.
  test_list = []

  paths = Paths(config)
  build_dir = paths.SrcRelPath(paths.build_dir)
  target_os = config.target_os

  verbose_flags = verbose_count * ["--verbose"]

  # Utility functions ----------------------------------------------------------

  # Call this to determine if a test matching classes this_tests_types should
  # run: e.g., ShouldRunTest(Config.TEST_TYPE_DEFAULT, "sky") returns true if
  # the test list being requested specifies the default set or the "sky" set.
  def ShouldRunTest(*this_tests_types):
    return not types_to_run.isdisjoint(this_tests_types)

  # Call this to add the given command to the test list.
  # |env| is an optional dictionary to be used to update the environment for
  # the test process.
  def AddEntry(name, command, env=None):
    if config.sanitizer == Config.SANITIZER_ASAN:
      command = (["python", os.path.join("mojo", "tools",
                                         "run_command_through_symbolizer.py")] +
                 command)
    test_list.append({"name": name, "command": command, "env" : env})

  # Call this to add the given command to the test list. If appropriate, the
  # command will be run under xvfb.
  def AddXvfbEntry(name, command):
    real_command = ["python"]
    if config.target_os == Config.OS_LINUX:
      real_command += ["./testing/xvfb.py", paths.SrcRelPath(paths.build_dir)]
    real_command += command
    AddEntry(name, real_command)

  # ----------------------------------------------------------------------------

  # TODO(vtl): Currently, we only know how to run tests for Android, Linux, or
  # Windows.
  if target_os not in (Config.OS_ANDROID, Config.OS_LINUX, Config.OS_WINDOWS,
                       Config.OS_IOS):
    return test_list

  # Tests run by default -------------------------------------------------------

  # C++ unit tests:
  if ShouldRunTest(Config.TEST_TYPE_DEFAULT, Config.TEST_TYPE_UNIT):
    AddXvfbEntry("Unit tests",
                 [os.path.join("mojo", "tools", "test_runner.py"),
                  os.path.join("mojo", "tools", "data", "unittests"),
                  build_dir] + verbose_flags)

  # C++ app tests:
  if ShouldRunTest(Config.TEST_TYPE_DEFAULT, "app"):
    AddXvfbEntry("App tests",
                 [os.path.join("mojo", "tools", "apptest_runner.py"),
                  os.path.join("mojo", "tools", "data", "apptests"),
                  build_dir] + verbose_flags)
    # Fusl app tests (Linux excluding Android):
    if (target_os == Config.OS_LINUX):
      AddXvfbEntry("Fusl app tests",
                   [os.path.join("mojo", "tools", "apptest_runner.py"),
                    os.path.join("mojo", "tools", "data", "fusl_apptests"),
                    build_dir] + verbose_flags)
    # Non-SFI NaCl app tests (Linux including Android, no asan):
    if config.sanitizer != Config.SANITIZER_ASAN:
      AddXvfbEntry("Non-SFI NaCl App tests",
                   [os.path.join("mojo", "tools", "apptest_runner.py"),
                    os.path.join("mojo", "tools", "data",
                                 "nacl_nonsfi_apptests"),
                    build_dir] + verbose_flags)

  # Go unit tests (Linux-only):
  if (target_os == Config.OS_LINUX and
      config.sanitizer != Config.SANITIZER_ASAN and
      ShouldRunTest(Config.TEST_TYPE_DEFAULT, Config.TEST_TYPE_UNIT, "go")):
    # Go system tests:
    AddEntry("Go system tests",
             [os.path.join(build_dir, "obj", "mojo", "go", "system_test")],
             env={'GODEBUG' : 'cgocheck=2'})

    # Pure Go unit tests:
    assert paths.go_tool_path is not None
    go_tool = paths.go_tool_path
    AddEntry("Go unit tests",
             ["python", os.path.join("mojo", "tools", "run_pure_go_tests.py"),
              go_tool, os.path.join("mojo", "tools", "data", "gotests")])

  # Python unit tests:
  if ShouldRunTest(Config.TEST_TYPE_DEFAULT, Config.TEST_TYPE_UNIT, "python"):
    AddEntry("Python unit tests",
             ["python", os.path.join("mojo", "tools",
              "run_mojo_python_tests.py")])

  # Python bindings tests (Linux-only):
  # See http://crbug.com/438781 for details on asan exclusion.
  if (target_os == Config.OS_LINUX and
      ShouldRunTest(Config.TEST_TYPE_DEFAULT, Config.TEST_TYPE_UNIT,
                    "python") and
      config.sanitizer != Config.SANITIZER_ASAN):
    AddEntry("Python bindings tests",
             ["python",
              os.path.join("mojo", "tools",
                           "run_mojo_python_bindings_tests.py"),
              "--build-dir=" + build_dir])

    AddEntry("Mojom translator python tests",
             ["python",
              os.path.join("mojo", "tools",
                           "run_mojom_translator_python_tests.py"),
              "--build-dir=" + build_dir])

  # Observatory tests (Linux-only):
  if target_os == Config.OS_LINUX and ShouldRunTest(Config.TEST_TYPE_DEFAULT):
    AddEntry("Dart Observatory tests",
             ["python",
              os.path.join("mojo", "dart", "unittests", "observatory_tester",
                           "runner.py"),
              "--build-dir=" + build_dir,
              "--dart-exe=third_party/dart-sdk/dart-sdk/bin/dart"])
    AddEntry("Dart HTTP Load test",
           ["python",
            os.path.join("mojo", "dart", "unittests", "http_load_test",
                         "runner.py"),
            "--build-dir=" + build_dir,
            "--dart-exe=third_party/dart-sdk/dart-sdk/bin/dart"])

  if target_os == Config.OS_LINUX:
    # Dart mojom package generate.dart script tests:
    if ShouldRunTest(Config.TEST_TYPE_DEFAULT):
      AddEntry("Dart mojom package generate tests",
          [os.path.join("third_party", "dart-sdk", "dart-sdk", "bin", "dart"),
           "--checked",
           "-p", os.path.join(build_dir, "gen", "dart-pkg", "packages"),
           os.path.join(
             "mojo", "dart", "packages", "mojom", "test",
             "generate_test.dart")])

    if ShouldRunTest(Config.TEST_TYPE_DEFAULT):
      AddEntry("Dart mojo package can be imported by a non-mojo embedder",
          [os.path.join("third_party", "dart-sdk", "dart-sdk", "bin", "dart"),
           "--checked",
           "-p", os.path.join(build_dir, "gen", "dart-pkg", "packages"),
           os.path.join(
             "mojo", "dart", "packages", "mojo", "test",
             "standalone_test.dart")])

    # Tests of Dart examples.
    if ShouldRunTest(Config.TEST_TYPE_DEFAULT):
      AddEntry("Dart examples tests",
          ["python", os.path.join("examples", "dart", "example_tests.py"),
           "--build-dir", build_dir])

    # Test of Dart's snapshotter.
    if ShouldRunTest(Config.TEST_TYPE_DEFAULT):
      AddEntry("Dart snapshotter test",
          ["python",
           os.path.join("mojo", "dart", "embedder", "snapshotter", "test",
                        "dart_snapshotter_test.py"),
           "--build-dir=" + build_dir])

  # Dart analyzer test
  if ShouldRunTest(Config.TEST_TYPE_DEFAULT, "analyze-dart"):
    AddEntry("Dart Package Static Analysis",
        ["python",
         os.path.join("mojo", "public", "tools", "dart_pkg_static_analysis.py"),
         "--dart-pkg-dir=" + os.path.join(build_dir, "gen", "dart-pkg"),
         "--dart-sdk=" + os.path.join("third_party", "dart-sdk", "dart-sdk"),
         "--package-root=" + os.path.join(build_dir,
                                          "gen",
                                          "dart-pkg",
                                          "packages")])

  # Perf tests -----------------------------------------------------------------

  bot_name = "linux_%s" % ("debug" if config.is_debug else "release")

  if target_os == Config.OS_LINUX and ShouldRunTest(Config.TEST_TYPE_PERF):
    test_names = ["mojo_public_system_perftests",
                  "mojo_public_bindings_perftests",
                  "mojo_edk_system_perftests"]

    for test_name in test_names:
      command = ["python",
                 os.path.join("mojo", "tools", "perf_test_runner.py"),
                 "--upload",
                 "--server-url", _PERFORMANCE_DASHBOARD_URL,
                 "--bot-name", bot_name,
                 "--test-name", test_name,
                 "--perf-data-path",
                 os.path.join(build_dir, test_name + "_perf.log")]
      if config.values.get("builder_name"):
        command += ["--builder-name", config.values["builder_name"]]
      if config.values.get("build_number"):
        command += ["--build-number", config.values["build_number"]]
      if config.values.get("master_name"):
        command += ["--master-name", config.values["master_name"]]
      command += [os.path.join(build_dir, test_name)]

      AddEntry(test_name, command)

  # Benchmarks -----------------------------------------------------------------

  if target_os == Config.OS_LINUX and ShouldRunTest(Config.TEST_TYPE_PERF):
    spec_files = ["benchmarks", "rtt_benchmarks"]
    aggregate_run_count = 3

    for spec_file in spec_files:
      command = ["python",
                 os.path.join("mojo", "devtools", "common", "mojo_benchmark"),
                 os.path.join("mojo", "tools", "data", spec_file),
                 "--aggregate", aggregate_run_count,
                 "--upload",
                 "--server-url", _PERFORMANCE_DASHBOARD_URL,
                 "--bot-name", bot_name,
                 "--test-name", "mojo_benchmarks"]

      if config.values.get("builder_name"):
        command += ["--builder-name", config.values["builder_name"]]
      if config.values.get("build_number"):
        command += ["--build-number", config.values["build_number"]]
      if config.values.get("master_name"):
        command += ["--master-name", config.values["master_name"]]

      if not config.is_debug:
        command += ["--release"]

      AddEntry("benchmarks: " + spec_file, command)

  # Integration tests ----------------------------------------------------------

  if target_os == Config.OS_ANDROID and ShouldRunTest(
      Config.TEST_TYPE_DEFAULT, Config.TEST_TYPE_INTEGRATION):
    AddEntry("Integration test (MojoTest)",
             ["python",
              os.path.join("build", "android", "test_runner.py"),
              "instrumentation",
              "--test-apk=MojoTest",
              "--output-directory=%s" % build_dir,
              "--test_data=bindings:mojo/public/interfaces/bindings/tests/data"]
             + verbose_flags)

  return test_list


def main():
  parser = argparse.ArgumentParser(description="Gets tests to execute.")
  parser.add_argument("config_file", metavar="config.json",
                      type=argparse.FileType("rb"),
                      help="Input JSON file with test configuration.")
  parser.add_argument("test_list_file", metavar="test_list.json", nargs="?",
                      type=argparse.FileType("wb"), default=sys.stdout,
                      help="Output JSON file with test list.")
  args = parser.parse_args()

  config = Config(**json.load(args.config_file))
  test_list = GetTestList(config)
  json.dump(test_list, args.test_list_file, indent=2)
  args.test_list_file.write("\n")

  return 0


if __name__ == "__main__":
  sys.exit(main())
