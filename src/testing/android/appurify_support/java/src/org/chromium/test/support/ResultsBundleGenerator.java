// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.test.support;

import android.app.Instrumentation;
import android.os.Bundle;

import java.util.Map;

/**
 * Creates a results Bundle.
 */
public interface ResultsBundleGenerator {
    /**
     * Holds the results of a test.
     */
    public static interface TestResult {
        /**
         * Returns the test class name.
         */
        public String getTestClass();

        /**
         * Returns the test case name.
         */
        public String getTestName();

        /**
         * Retunrs the index of the test within the suite.
         */
        public int getTestIndex();

        /**
         * Returns a message for the test.
         */
        public String getMessage();

        /**
         * Returns the test case log.
         */
        public String getLog();

        /**
         * Returns the status of the test.
         */
        public TestStatus getStatus();
    }

    /** Indicates the state of a test.
     */
    public static enum TestStatus { PASSED, FAILED, ERROR, UNKNOWN }

    /** Sends results to the instrumentation framework.

        @returns a summary Bundle that should be used to terminate the instrumentation.

        Note: actual bundle content and format may vary.

        @param instrumentation The application instrumentation used for testing.
        @param rawResults A map between test names and test results.
     */
    Bundle sendResults(Instrumentation instrumentation, Map<String, TestResult> rawResults);
}
