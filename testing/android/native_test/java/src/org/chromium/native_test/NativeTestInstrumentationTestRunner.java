// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.native_test;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;

import org.chromium.base.Log;
import org.chromium.test.support.ResultsBundleGenerator;
import org.chromium.test.support.RobotiumBundleGenerator;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 *  An Instrumentation that runs tests based on NativeTestActivity.
 */
public class NativeTestInstrumentationTestRunner extends Instrumentation {

    public static final String EXTRA_NATIVE_TEST_ACTIVITY =
            "org.chromium.native_test.NativeTestInstrumentationTestRunner."
                    + "NativeTestActivity";

    private static final String TAG = "cr.native_test";

    private static final String DEFAULT_NATIVE_TEST_ACTIVITY =
            "org.chromium.native_test.NativeUnitTestActivity";
    private static final Pattern RE_TEST_OUTPUT =
            Pattern.compile("\\[ *([^ ]*) *\\] ?([^ \\.]+)\\.([^ \\.]+)($| .*)");

    private ResultsBundleGenerator mBundleGenerator = new RobotiumBundleGenerator();
    private String mCommandLineFile;
    private String mCommandLineFlags;
    private String mNativeTestActivity;
    private Bundle mLogBundle = new Bundle();
    private File mStdoutFile;

    private class NativeTestResult implements ResultsBundleGenerator.TestResult {
        private String mTestClass;
        private String mTestName;
        private int mTestIndex;
        private StringBuilder mLogBuilder = new StringBuilder();
        private ResultsBundleGenerator.TestStatus mStatus;

        private NativeTestResult(String testClass, String testName, int index) {
            mTestClass = testClass;
            mTestName = testName;
            mTestIndex = index;
        }

        @Override
        public String getTestClass() {
            return mTestClass;
        }

        @Override
        public String getTestName() {
            return mTestName;
        }

        @Override
        public int getTestIndex() {
            return mTestIndex;
        }

        @Override
        public String getMessage() {
            return mStatus.toString();
        }

        @Override
        public String getLog() {
            return mLogBuilder.toString();
        }

        public void appendToLog(String logLine) {
            mLogBuilder.append(logLine);
            mLogBuilder.append("\n");
        }

        @Override
        public ResultsBundleGenerator.TestStatus getStatus() {
            return mStatus;
        }

        public void setStatus(ResultsBundleGenerator.TestStatus status) {
            mStatus = status;
        }
    }

    @Override
    public void onCreate(Bundle arguments) {
        mCommandLineFile = arguments.getString(NativeTestActivity.EXTRA_COMMAND_LINE_FILE);
        mCommandLineFlags = arguments.getString(NativeTestActivity.EXTRA_COMMAND_LINE_FLAGS);
        mNativeTestActivity = arguments.getString(EXTRA_NATIVE_TEST_ACTIVITY);
        if (mNativeTestActivity == null) mNativeTestActivity = DEFAULT_NATIVE_TEST_ACTIVITY;

        try {
            mStdoutFile = File.createTempFile(
                    ".temp_stdout_", ".txt", Environment.getExternalStorageDirectory());
            Log.i(TAG, "stdout file created: %s", mStdoutFile.getAbsolutePath());
        } catch (IOException e) {
            Log.e(TAG, "Unable to create temporary stdout file.", e);
            finish(Activity.RESULT_CANCELED, new Bundle());
            return;
        }
        start();
    }

    @Override
    public void onStart() {
        super.onStart();
        Bundle results = runTests();
        finish(Activity.RESULT_OK, results);
    }

    /** Runs the tests in the NativeTestActivity and returns a Bundle containing the results.
     */
    private Bundle runTests() {
        Log.i(TAG, "Creating activity.");
        Activity activityUnderTest = startNativeTestActivity();

        try {
            while (!activityUnderTest.isFinishing()) {
                Thread.sleep(100);
            }
        } catch (InterruptedException e) {
            Log.e(TAG, "Interrupted while waiting for activity to be destroyed: ", e);
        }

        Log.i(TAG, "Getting results.");
        Map<String, ResultsBundleGenerator.TestResult> results = parseResults(activityUnderTest);

        Log.i(TAG, "Parsing results and generating output.");
        return mBundleGenerator.sendResults(this, results);
    }

    /** Starts the NativeTestActivty.
     */
    private Activity startNativeTestActivity() {
        Intent i = new Intent(Intent.ACTION_MAIN);
        i.setComponent(new ComponentName(getContext().getPackageName(), mNativeTestActivity));
        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (mCommandLineFile != null) {
            Log.i(TAG, "Passing command line file extra: %s", mCommandLineFile);
            i.putExtra(NativeTestActivity.EXTRA_COMMAND_LINE_FILE, mCommandLineFile);
        }
        if (mCommandLineFlags != null) {
            Log.i(TAG, "Passing command line flag extra: %s", mCommandLineFlags);
            i.putExtra(NativeTestActivity.EXTRA_COMMAND_LINE_FLAGS, mCommandLineFlags);
        }
        i.putExtra(NativeTestActivity.EXTRA_STDOUT_FILE, mStdoutFile.getAbsolutePath());
        return startActivitySync(i);
    }

    /**
     *  Generates a map between test names and test results from the instrumented Activity's
     *  output.
     */
    private Map<String, ResultsBundleGenerator.TestResult> parseResults(
            Activity activityUnderTest) {
        Map<String, ResultsBundleGenerator.TestResult> results =
                new LinkedHashMap<String, ResultsBundleGenerator.TestResult>();

        BufferedReader r = null;

        try {
            if (mStdoutFile == null || !mStdoutFile.exists()) {
                Log.e(TAG, "Unable to find stdout file.");
                return results;
            }

            r = new BufferedReader(new InputStreamReader(
                    new BufferedInputStream(new FileInputStream(mStdoutFile))));

            NativeTestResult testResult = null;
            int testNum = 0;
            for (String l = r.readLine(); l != null && !l.equals("<<ScopedMainEntryLogger");
                    l = r.readLine()) {
                Matcher m = RE_TEST_OUTPUT.matcher(l);
                if (m.matches()) {
                    if (m.group(1).equals("RUN")) {
                        String testClass = m.group(2);
                        String testName = m.group(3);
                        testResult = new NativeTestResult(testClass, testName, ++testNum);
                        results.put(String.format("%s.%s", m.group(2), m.group(3)), testResult);
                    } else if (testResult != null && m.group(1).equals("FAILED")) {
                        testResult.setStatus(ResultsBundleGenerator.TestStatus.FAILED);
                        testResult = null;
                    } else if (testResult != null && m.group(1).equals("OK")) {
                        testResult.setStatus(ResultsBundleGenerator.TestStatus.PASSED);
                        testResult = null;
                    }
                } else if (testResult != null) {
                    // We are inside a test. Let's collect log data in case there is a failure.
                    testResult.appendToLog(l);
                }
                Log.i(TAG, l);
            }

        } catch (FileNotFoundException e) {
            Log.e(TAG, "Couldn't find stdout file file: ", e);
        } catch (IOException e) {
            Log.e(TAG, "Error handling stdout file: ", e);
        } finally {
            if (r != null) {
                try {
                    r.close();
                } catch (IOException e) {
                    Log.e(TAG, "Error while closing stdout reader.", e);
                }
            }
            if (mStdoutFile != null) {
                if (!mStdoutFile.delete()) {
                    Log.e(TAG, "Unable to delete %s", mStdoutFile.getAbsolutePath());
                }
            }
        }
        return results;
    }

}
