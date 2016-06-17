// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.NfcEvent;

import org.chromium.base.ApplicationStatus;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.nfc.Nfc;
import org.chromium.mojom.nfc.NfcData;
import org.chromium.mojom.nfc.NfcTransmission;

import java.util.ArrayList;
import java.util.List;

/**
 * Android implementation of Nfc.
 */
class NfcImpl implements Nfc, NfcAdapter.OnNdefPushCompleteCallback,
                         NfcAdapter.CreateNdefMessageCallback {
    private final NfcAdapter mNfcAdapter;
    private final String mPackageName;
    private final String mRequestorUrl;
    private final List<NfcTransmissionInfo> mNfcTransmissionInfos;
    private List<NfcTransmissionInfo> mLastMessageNfcTransmissionInfos;

    NfcImpl(NfcAdapter nfcAdapter, String packageName, String requestorUrl) {
        mNfcAdapter = nfcAdapter;
        mPackageName = packageName;
        mRequestorUrl = requestorUrl;
        mNfcTransmissionInfos = new ArrayList<NfcTransmissionInfo>();
        mLastMessageNfcTransmissionInfos = new ArrayList<NfcTransmissionInfo>();
    }

    @Override
    public void close() {}

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void transmitOnNextConnection(NfcData nfcData,
            org.chromium.mojo.bindings.InterfaceRequest<NfcTransmission> request,
            TransmitOnNextConnectionResponse transmitOnNextConnectionResponse) {
        NfcTransmissionInfo nfcTransmissionInfo =
                new NfcTransmissionInfo(nfcData, transmitOnNextConnectionResponse);
        mNfcTransmissionInfos.add(nfcTransmissionInfo);
        NfcTransmission nfcTransmission = new NfcTransmissionImpl(nfcTransmissionInfo);
        NfcTransmission.MANAGER.bind(nfcTransmission, request);
        mNfcAdapter.setNdefPushMessageCallback(
                this, ApplicationStatus.getLastTrackedFocusedActivity());
        mNfcAdapter.setOnNdefPushCompleteCallback(
                this, ApplicationStatus.getLastTrackedFocusedActivity());
    }

    @Override
    public void register() {
        NfcDbManager.getInstance().addRegisteredApplication(mRequestorUrl);
    }

    @Override
    public void unregister() {
        NfcDbManager.getInstance().removeRegisteredApplication(mRequestorUrl);
    }

    @Override
    public NdefMessage createNdefMessage(NfcEvent event) {
        if (mNfcTransmissionInfos.isEmpty()) {
            return null;
        }
        mLastMessageNfcTransmissionInfos =
                new ArrayList<NfcTransmissionInfo>(mNfcTransmissionInfos);

        NdefRecord[] records = new NdefRecord[mLastMessageNfcTransmissionInfos.size() + 1];
        for (int i = 0, size = mLastMessageNfcTransmissionInfos.size(); i < size; i++) {
            records[i] = NdefRecord.createMime(
                    "mojo/data", mLastMessageNfcTransmissionInfos.get(i).mNfcData.data);
        }
        records[records.length - 1] = NdefRecord.createApplicationRecord(mPackageName);
        return new NdefMessage(records);
    }

    @Override
    public void onNdefPushComplete(NfcEvent event) {
        for (NfcTransmissionInfo nfcTransmissionInfo : mLastMessageNfcTransmissionInfos) {
            nfcTransmissionInfo.mTransmitOnNextConnectionResponse.call(true);
            removeNfcTransmissionInfo(nfcTransmissionInfo);
        }
    }

    void removeNfcTransmissionInfo(NfcTransmissionInfo nfcTransmissionInfo) {
        mNfcTransmissionInfos.remove(nfcTransmissionInfo);
        if (mNfcTransmissionInfos.isEmpty()) {
            mNfcAdapter.setNdefPushMessageCallback(
                    null, ApplicationStatus.getLastTrackedFocusedActivity());
            mNfcAdapter.setOnNdefPushCompleteCallback(
                    null, ApplicationStatus.getLastTrackedFocusedActivity());
        }
    }

    class NfcTransmissionInfo {
        final NfcData mNfcData;
        final TransmitOnNextConnectionResponse mTransmitOnNextConnectionResponse;

        NfcTransmissionInfo(NfcData nfcData,
                TransmitOnNextConnectionResponse transmitOnNextConnectionResponse) {
            mNfcData = nfcData;
            mTransmitOnNextConnectionResponse = transmitOnNextConnectionResponse;
        }
    }

    class NfcTransmissionImpl implements NfcTransmission {
        private final NfcTransmissionInfo mNfcTransmissionInfo;

        public NfcTransmissionImpl(NfcTransmissionInfo nfcTransmissionInfo) {
            mNfcTransmissionInfo = nfcTransmissionInfo;
        }

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}

        @Override
        public void cancel() {
            removeNfcTransmissionInfo(mNfcTransmissionInfo);
        }
    }
}
