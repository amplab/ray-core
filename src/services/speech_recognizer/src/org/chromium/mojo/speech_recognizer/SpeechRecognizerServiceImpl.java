// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.speech_recognizer;

import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.text.TextUtils;
import android.util.Log;

import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;

/**
 * Implementation of org.chromium.mojom.mojo.SpeechRecognizerService.
 */
final class SpeechRecognizerServiceImpl implements SpeechRecognizerService {
    private static final String TAG = "SpeechRecognizerServiceImpl";

    private final Context mContext;
    private final Handler mMainHandler;

    private SpeechRecognizer mSpeechRecognizer;

    private AudioManager mAudioManager;

    private SpeechRecognizerListener mListener;

    private boolean mReadyForSpeech = false;

    SpeechRecognizerServiceImpl(Context context) {
        mContext = context;
        mMainHandler = new Handler(mContext.getMainLooper());
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                mSpeechRecognizer = SpeechRecognizer.createSpeechRecognizer(mContext);
                mSpeechRecognizer.setRecognitionListener(new SpeechListener());
            }
        });

        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
    }

    /**
     * Returns a list of utterances from the user's speech input.
     *
     * @see SpeechRecognizerService#listen(SpeechRecognizerListener)
     */
    @Override
    public void listen(SpeechRecognizerListener listener) {
        if (mListener != null) {
            listener.onRecognizerError(Error.RECOGNIZER_BUSY);
            return;
        }

        mListener = listener;

        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "startListening");

                // Ignore the result of requestAudioFocus() since a failure only indicates that
                // we'll be listening in a slightly noise environment.
                mAudioManager.requestAudioFocus(null, AudioManager.USE_DEFAULT_STREAM_TYPE,
                        AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE);

                startListening();
            }
        });
    }

    /**
     * @see SpeechRecognizerService#stopListening()
     */
    @Override
    public void stopListening() {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                mSpeechRecognizer.stopListening();
            }
        });
    }

    /**
     * When connection is closed, we stop receiving location updates.
     *
     * @see org.chromium.mojo.bindings.Interface#close()
     */
    @Override
    public void close() {}

    /**
     * @see org.chromium.mojo.bindings.Interface#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {
        throw e;
    }

    private void startListening() {
        Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        intent.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true);

        mReadyForSpeech = false;
        mSpeechRecognizer.startListening(intent);
    }

    class SpeechListener implements RecognitionListener {
        @Override
        public void onReadyForSpeech(Bundle params) {
            Log.d(TAG, "onReadyForSpeech");
            mReadyForSpeech = true;
        }

        @Override
        public void onBeginningOfSpeech() {}

        @Override
        public void onRmsChanged(float rmsdB) {
            if (mListener != null) {
                mListener.onSoundLevelChanged(rmsdB);
            }
        }

        @Override
        public void onBufferReceived(byte[] buffer) {}

        @Override
        public void onEndOfSpeech() {}

        @Override
        public void onError(int error) {
            Log.d(TAG, "onError " + error);

            // There is a bug in Google Now that causes speech recognition to prematurely
            // fail if "OK Google Detection" is set to listen "from any screen".
            // (See http://b.android.com/179293).
            if (!mReadyForSpeech && error == SpeechRecognizer.ERROR_NO_MATCH) {
                mSpeechRecognizer.cancel();
                startListening();
                return;
            }

            mAudioManager.abandonAudioFocus(null);
            if (mListener != null) {
                // The enum in the mojom for SpeechRecognizerService matches the
                // errors that come from Android's RecognizerService.
                mListener.onRecognizerError(error);
                mListener = null;
            }
        }

        @Override
        public void onResults(Bundle results) {
            mAudioManager.abandonAudioFocus(null);
            processResults(results, true);
            mListener = null;
        }

        @Override
        public void onPartialResults(Bundle partialResults) {
            processResults(partialResults, false);
        }

        @Override
        public void onEvent(int eventType, Bundle params) {}

        private void processResults(Bundle results, boolean complete) {
            ArrayList<String> utterances =
                    results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
            float[] confidences = results.getFloatArray(SpeechRecognizer.CONFIDENCE_SCORES);

            ArrayList<UtteranceCandidate> candidates = new ArrayList<UtteranceCandidate>();
            for (int i = 0; i < utterances.size(); i++) {
                UtteranceCandidate candidate = new UtteranceCandidate();
                candidate.text = utterances.get(i);
                if (confidences != null) {
                    candidate.confidenceScore = confidences[i];
                } else {
                    candidate.confidenceScore = 0;
                }
                if (!TextUtils.isEmpty(candidate.text)) {
                    candidates.add(candidate);
                }
            }

            if (mListener != null) {
                mListener.onResults(
                        candidates.toArray(new UtteranceCandidate[candidates.size()]), complete);
            }
        }
    }
}
