#!mojo mojo:js_content_handler
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Simple JavaScript REPL example, using TerminalClient.
//
// To run this, do something like:
//
//   $ out/Debug/mojo_shell "mojo:launcher mojo:moterm_example_app"
//
// At the moterm_example_app prompt:
//
//   :) file:///absolute/path/to/repl.js
//
// Then, at the repl.js prompt, enter JavaScript, e.g.:
//
//   > 1 + 2
//   3
//   > function f(x, y) { return x + y; }
//   undefined
//   > f("abc", 123)
//   "abc123"
//
// (Note that multiple simultaneous REPLs will share the same global state, that
// the global state will persist, etc. This is really just intended as a demo!)

define("main", [
  "mojo/services/public/js/application",
  "mojo/services/files/interfaces/types.mojom",
  "mojo/services/terminal/interfaces/terminal_client.mojom",
], function(application, files_types, terminal_client) {
  const Application = application.Application;
  const TerminalClient = terminal_client.TerminalClient;
  const Whence = files_types.Whence;

  function stringToBytes(s) {
    return s.split("").map(function (c) { return c.charCodeAt(0); });
  }

  class Repl {
    constructor(terminal) {
      this.terminal_ = terminal;
    }

    start() {
      // TODO(vtl): Handle errors.
      var self = this;
      this.terminal_.write(stringToBytes("> "), 0, Whence.FROM_CURRENT)
          .then(function (result) { self.didWritePrompt_(result); });
    }

    didWritePrompt_(result) {
      // TODO(vtl): Handle errors.
      var self = this;
      this.terminal_.read(1000, 0, Whence.FROM_CURRENT)
          .then(function (result) { self.didRead_(result); });
    }

    didRead_(result) {
      // TODO(vtl): Handle errors.
      var s = String.fromCharCode.apply(String, result.bytes_read).trim();
      var r;
      try {
        // Note: Indirectly calling eval as follows leads to the evaluation
        // working at global scope (as of ECMAScript 5). (This allows us to
        // persist state across evals without doing any work.)
        var geval = eval;
        r = geval(s);
      } catch (e) {
        r = e;
      }
      var resultString;
      try {
        if (typeof(r) == "string") {
          resultString = "\"" + r + "\"";
        } else if (Array.isArray(r)) {
          resultString = "[" + r + "]";
        } else {
          resultString = "" + r;
        }
      } catch (e) {
        resultString = "Oops!";
      }
      var self = this;
      this.terminal_.write(stringToBytes(resultString + "\n"), 0,
                           Whence.FROM_CURRENT)
          .then(function (result) { self.didWriteResult_(result); });
    }

    didWriteResult_(result) {
      this.start();
    }
  }

  class TerminalClientImpl {
    connectToTerminal(terminal) {
      (new Repl(terminal)).start();
    }
  }

  class ReplApp extends Application {
    acceptConnection(initiatorURL, initiatorServiceExchange) {
      initiatorServiceExchange.provideService(TerminalClient,
                                              TerminalClientImpl);
    }
  }

  return ReplApp;
});
