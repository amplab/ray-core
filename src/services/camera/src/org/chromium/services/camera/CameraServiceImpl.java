// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.camera;

import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;

import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.DataPipe;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;
import org.chromium.mojom.mojo.CameraService;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.concurrent.Semaphore;

/**
 * Implementation of AuthenticationService from services/camera/camera.mojom
 */
public class CameraServiceImpl implements CameraService {
    private final Core mCore;
    private final Context mContext;
    private static final String TAG = "CameraServiceImpl";

    private Semaphore mCameraOpenCloseLock = new Semaphore(1);
    private CameraDevice mCameraDevice;
    private HandlerThread mBackgroundThread;
    private Handler mHandler;

    private ImageReader mImageReader;
    private DataPipe.ProducerHandle mProducerHandle;

    public CameraServiceImpl(Context context, Core core) {
        mContext = context;
        mCore = core;
        startBackgroundThread();
    }

    private final ImageReader.OnImageAvailableListener mOnImageAvailableListener =
            new ImageReader.OnImageAvailableListener() {
                @Override
                public void onImageAvailable(ImageReader reader) {
                    DataPipe.ProducerHandle handle = null;
                    synchronized (CameraServiceImpl.this) {
                        handle = mProducerHandle;
                        mProducerHandle = null;
                    }
                    try (Image img = reader.acquireLatestImage()) {
                        if (handle != null) {
                            // TODO: Dont write the image data as a single block.
                            ByteBuffer buffer = img.getPlanes()[0].getBuffer();
                            handle.writeData(buffer, DataPipe.WriteFlags.none());
                        }
                    } catch (MojoException e) {
                        Log.e(TAG, "Failed to write to producer", e);
                    } finally {
                        if (handle != null) {
                            handle.close();
                        }
                    }
                }
            };

    public void openCamera() {
        CameraManager manager = (CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
        try {
            for (String cameraId : manager.getCameraIdList()) {
                CameraCharacteristics characteristics =
                        manager.getCameraCharacteristics(cameraId);
                StreamConfigurationMap map = characteristics
                        .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                Size size = chooseVideoSize(map.getOutputSizes(ImageFormat.JPEG));
                // We don't use a front facing camera in this sample.
                Integer facing = characteristics.get(CameraCharacteristics.LENS_FACING);
                if (facing != null && facing == CameraCharacteristics.LENS_FACING_FRONT) {
                    continue;
                }
                mImageReader = ImageReader
                        .newInstance(size.getWidth(), size.getHeight(),
                                     ImageFormat.JPEG, 100/*fps*/);
                mImageReader.setOnImageAvailableListener(mOnImageAvailableListener, mHandler);
                mCameraOpenCloseLock.acquire();
                try {
                    manager.openCamera(cameraId, mCameraStateCallback, mHandler);
                } catch (Exception e) {
                    mCameraOpenCloseLock.release();
                    Log.e(TAG, "Failed to openCamera", e);
                }
                break;
            }
        } catch (CameraAccessException e) {
            Log.e(TAG, "Failed to access camera characteristics", e);
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera opening.");
        }
    }

    private void closeCamera() {
        try {
            mCameraOpenCloseLock.acquire();
            try {
                if (mImageReader != null) {
                    mImageReader.close();
                    mImageReader = null;
                }
                if (mCameraDevice != null) {
                    mCameraDevice.close();
                    mCameraDevice = null;
                }
            } finally {
                mCameraOpenCloseLock.release();
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera opening.");
        }
    }

    private void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("CameraServiceImplThread");
        mBackgroundThread.start();
        mHandler = new Handler(mBackgroundThread.getLooper());
    }

    private void stopBackgroundThread() {
        mBackgroundThread.quitSafely();
        try {
            mBackgroundThread.join();
            mBackgroundThread = null;
            mHandler = null;
        } catch (InterruptedException e) {
            Log.e(TAG, "Failed to stop background thread", e);
        }
    }

    private Size chooseVideoSize(Size[] choices) {
        for (Size size : choices) {
            // 1080p resolution
            if (size.getWidth() == size.getHeight() * 16 / 9 && size.getHeight() <= 1080) {
                return size;
            }
        }
        return choices[choices.length - 1];
    }

    private final CameraDevice.StateCallback mCameraStateCallback =
            new CameraDevice.StateCallback() {
                @Override
                public void onOpened(CameraDevice device) {
                    mCameraDevice = device;
                    startPreview();
                    mCameraOpenCloseLock.release();
                }

                @Override
                public void onDisconnected(CameraDevice cameraDevice) {
                    mCameraOpenCloseLock.release();
                }

                @Override
                public void onError(CameraDevice cameraDevice, int error) {
                    mCameraOpenCloseLock.release();
                    Log.e(TAG, "Failed to connect to camera:" + error);
                }
            };

    private void startPreview() {
        try {
            final CaptureRequest.Builder request =
                    mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            request.addTarget(mImageReader.getSurface());
            mCameraDevice.createCaptureSession(
                    Arrays.asList(mImageReader.getSurface()),
                    new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(CameraCaptureSession session) {
                            request.set(CaptureRequest.CONTROL_MODE,
                                    CameraMetadata.CONTROL_MODE_AUTO);
                            try {
                                session.setRepeatingRequest(request.build(), null, mHandler);
                            } catch (CameraAccessException e) {
                                Log.e(TAG, "Failed to set repeating request", e);
                            }
                        }

                        @Override
                        public void onConfigureFailed(CameraCaptureSession session) {
                            Log.e(TAG, "Could not configure session capture");
                        }
                    }, mHandler);
        } catch (CameraAccessException e) {
            Log.e(TAG, "Failed to start preview for camera", e);
        }
    }

    @Override
    public void getLatestFrame(CameraService.GetLatestFrameResponse callback) {
        Pair<DataPipe.ProducerHandle, DataPipe.ConsumerHandle> handles = mCore.createDataPipe(null);
        callback.call(handles.second);
        synchronized (this) {
            if (mProducerHandle != null) {
                mProducerHandle.close();
            }
            mProducerHandle = handles.first;
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
    }

    @Override
    public void close() {
        closeCamera();
        synchronized (this) {
            if (mProducerHandle != null) {
                mProducerHandle.close();
                mProducerHandle = null;
            }
        }
    }

    public boolean cameraInUse() {
        return mImageReader != null;
    }

    public void cleanup() {
        stopBackgroundThread();
        close();
    }
}
