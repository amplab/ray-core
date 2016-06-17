// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.test.support;

import android.app.Instrumentation;
import android.os.Bundle;
import android.util.Log;

import java.util.Map;

/**
 * Creates a results bundle that emulates the one created by Robotium.
 */
public class RobotiumBundleGenerator implements ResultsBundleGenerator {

    private static final String TAG = "RobotiumBundleGenerator";

    // The constants below are copied from android.test.InstrumentationTestRunner.

    /**
     * This value, if stored with key {@link android.app.Instrumentation#REPORT_KEY_IDENTIFIER},
     * identifies RobotiumBundleGenerator as the source of the report.  This is sent with all
     * status messages.
     */
    public static final String REPORT_VALUE_ID = "RobotiumBundleGenerator";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * identifies the total number of tests that are being run.  This is sent with all status
     * messages.
     */
    public static final String REPORT_KEY_NUM_TOTAL = "numtests";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * identifies the sequence number of the current test.  This is sent with any status message
     * describing a specific test being started or completed.
     */
    public static final String REPORT_KEY_NUM_CURRENT = "current";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * identifies the name of the current test class.  This is sent with any status message
     * describing a specific test being started or completed.
     */
    public static final String REPORT_KEY_NAME_CLASS = "class";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * identifies the name of the current test.  This is sent with any status message
     * describing a specific test being started or completed.
     */
    public static final String REPORT_KEY_NAME_TEST = "test";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * reports the run time in seconds of the current test.
     */
    private static final String REPORT_KEY_RUN_TIME = "runtime";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * reports the number of total iterations of the current test.
     */
    private static final String REPORT_KEY_NUM_ITERATIONS = "numiterations";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * reports the guessed suite assignment for the current test.
     */
    private static final String REPORT_KEY_SUITE_ASSIGNMENT = "suiteassignment";
    /**
     * If included in the status or final bundle sent to an IInstrumentationWatcher, this key
     * identifies the path to the generated code coverage file.
     */
    private static final String REPORT_KEY_COVERAGE_PATH = "coverageFilePath";

    /**
     * The test is starting.
     */
    public static final int REPORT_VALUE_RESULT_START = 1;
    /**
     * The test completed successfully.
     */
    public static final int REPORT_VALUE_RESULT_OK = 0;
    /**
     * The test completed with an error.
     */
    public static final int REPORT_VALUE_RESULT_ERROR = -1;
    /**
     * The test completed with a failure.
     */
    public static final int REPORT_VALUE_RESULT_FAILURE = -2;
    /**
     * If included in the status bundle sent to an IInstrumentationWatcher, this key
     * identifies a stack trace describing an error or failure.  This is sent with any status
     * message describing a specific test being completed.
     */
    public static final String REPORT_KEY_STACK = "stack";

    public Bundle sendResults(Instrumentation instrumentation,
            Map<String, ResultsBundleGenerator.TestResult> rawResults) {
        int totalTests = rawResults.size();

        int testsPassed = 0;
        int testsFailed = 0;

        for (Map.Entry<String, ResultsBundleGenerator.TestResult> entry : rawResults.entrySet()) {
            ResultsBundleGenerator.TestResult result = entry.getValue();
            Bundle startBundle = new Bundle();
            startBundle.putString(Instrumentation.REPORT_KEY_IDENTIFIER, REPORT_VALUE_ID);
            startBundle.putString(REPORT_KEY_NAME_CLASS, result.getTestClass());
            startBundle.putString(REPORT_KEY_NAME_TEST, result.getTestName());
            startBundle.putInt(REPORT_KEY_NUM_CURRENT, result.getTestIndex());
            startBundle.putInt(REPORT_KEY_NUM_TOTAL, totalTests);
            startBundle.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                    String.format("\n%s.%s: ", result.getTestClass(), result.getTestName()));
            instrumentation.sendStatus(REPORT_VALUE_RESULT_START, startBundle);

            Bundle resultBundle = new Bundle(startBundle);
            resultBundle.putString(Instrumentation.REPORT_KEY_STREAMRESULT, result.getMessage());
            switch (result.getStatus()) {
                case PASSED:
                    ++testsPassed;
                    instrumentation.sendStatus(REPORT_VALUE_RESULT_OK, resultBundle);
                    break;
                case FAILED:
                    // TODO(jbudorick): Remove this log message once AMP execution and
                    // results handling has been stabilized.
                    Log.d(TAG, "FAILED: " + entry.getKey());
                    ++testsFailed;
                    resultBundle.putString(REPORT_KEY_STACK, result.getLog());
                    instrumentation.sendStatus(REPORT_VALUE_RESULT_FAILURE, resultBundle);
                    break;
                default:
                    Log.w(TAG, "Unhandled: " + entry.getKey() + ", "
                            + entry.getValue().toString());
                    resultBundle.putString(REPORT_KEY_STACK, result.getLog());
                    instrumentation.sendStatus(REPORT_VALUE_RESULT_ERROR, resultBundle);
                    break;
            }
        }

        StringBuilder resultBuilder = new StringBuilder();
        if (testsFailed > 0) {
            resultBuilder.append(
                    "\nFAILURES!!! Tests run: " + Integer.toString(rawResults.size())
                    + ", Failures: " + Integer.toString(testsFailed) + ", Errors: 0");
        } else {
            resultBuilder.append("\nOK (" + Integer.toString(testsPassed) + " tests)");
        }

        Bundle resultsBundle = new Bundle();
        resultsBundle.putString(Instrumentation.REPORT_KEY_STREAMRESULT,
                resultBuilder.toString());
        return resultsBundle;
    }
}
