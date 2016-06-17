// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.camera_roll;

import android.content.Context;
import android.os.Environment;
import android.util.Log;
import android.webkit.MimeTypeMap;

import org.chromium.mojo.application.ApplicationConnection;
import org.chromium.mojo.application.ApplicationDelegate;
import org.chromium.mojo.application.ApplicationRunner;
import org.chromium.mojo.application.ServiceFactoryBinder;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.DataPipe;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.MojoResult;
import org.chromium.mojo.system.Pair;
import org.chromium.mojom.mojo.CameraRollService;
import org.chromium.mojom.mojo.Photo;
import org.chromium.mojom.mojo.Shell;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Implementation of org.chromium.mojom.mojo.CameraRollService.
 */
class CameraRollServiceImpl implements CameraRollService {
    private static final String TAG = "CameraRollServiceImpl";

    private final Core mCore;
    private final Executor mExecutor;
    private ArrayList<File> mPhotos;

    class CopyToPipeJob implements Runnable {
        @SuppressWarnings("hiding")
        private static final String TAG = "CopyToPipeJob";

        private final File mPhotoFile;
        private DataPipe.ProducerHandle mProducer;

        CopyToPipeJob(File photoFile, DataPipe.ProducerHandle producerHandle) {
            mPhotoFile = photoFile;
            mProducer = producerHandle;
        }

        @Override
        public void run() {
            try (FileInputStream fileInputStream = new FileInputStream(mPhotoFile);
                    FileChannel channel = fileInputStream.getChannel()) {
                int bytesRead = 0;
                do {
                    try {
                        ByteBuffer buffer = mProducer.beginWriteData(0, DataPipe.WriteFlags.none());
                        bytesRead = channel.read(buffer);
                        mProducer.endWriteData(bytesRead == -1 ? 0 : bytesRead);
                    } catch (MojoException e) {
                        if (e.getMojoResult() == MojoResult.SHOULD_WAIT) {
                            mCore.wait(
                                    mProducer, Core.HandleSignals.WRITABLE, Core.DEADLINE_INFINITE);
                        } else if (e.getMojoResult() == MojoResult.FAILED_PRECONDITION) {
                            break;
                        } else {
                            throw e;
                        }
                    }
                } while (bytesRead != -1);
            } catch (IOException e) {
                Log.e(TAG, e.toString());
            } finally {
                mProducer.close();
            }
        }
    }

    CameraRollServiceImpl(Core core, Executor executor) {
        mCore = core;
        mExecutor = executor;
        constructPhotoGallery();
    }

    /**
     * Updates the list from which |getPhoto| returns its result.
     *
     * @see org.chromium.mojom.mojo.CameraRollService#update()
     */
    @Override
    public void update() {
        constructPhotoGallery();
    }

    /**
     * Returns the number of photos available in the camera roll.
     *
     * @see org.chromium.mojom.mojo.CameraRollService#getCount(GetCountResponse)
     */
    @Override
    public void getCount(CameraRollService.GetCountResponse callback) {
        callback.call(mPhotos.size());
    }

    /**
     * Returns the photo from the current snapshot of the camera roll at the requested index.
     *
     * @see org.chromium.mojom.mojo.CameraRollService#getPhoto(int, GetPhotoResponse)
     */
    @Override
    public void getPhoto(int index, CameraRollService.GetPhotoResponse callback) {
        if (index >= mPhotos.size()) {
            callback.call(null);
            return;
        }

        File photoFile = mPhotos.get(index);

        Photo photo = new Photo();

        try {
            String uniqueId = Long.toString(photoFile.lastModified()) + photoFile.getPath();
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            md.update(uniqueId.getBytes("UTF-8"));
            byte[] hash = md.digest();
            BigInteger bigInt = new BigInteger(1, hash);
            photo.uniqueId = bigInt.toString(16);
        } catch (NoSuchAlgorithmException e) {
            Log.e(TAG, e.toString());
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, e.toString());
        }

        Pair<DataPipe.ProducerHandle, DataPipe.ConsumerHandle> handles = mCore.createDataPipe(null);

        photo.content = handles.second;

        callback.call(photo);
        mExecutor.execute(new CopyToPipeJob(photoFile, handles.first));
    }

    private void constructPhotoGallery() {
        mPhotos = new ArrayList<File>();
        File dcimDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM);
        findAllImageFiles(dcimDir, mPhotos);
        Collections.sort(mPhotos, Collections.reverseOrder(new Comparator<File>() {
            @Override
            public int compare(File f1, File f2) {
                return Long.compare(f1.lastModified(), f2.lastModified());
            }
        }));
    }

    private void findAllImageFiles(File rootDir, ArrayList<File> outputImageFiles) {
        for (File file : rootDir.listFiles()) {
            if (file.isHidden()) {
                continue;
            }
            if (file.isDirectory()) {
                findAllImageFiles(file, outputImageFiles);
            }
            if (file.isFile()) {
                String extension = MimeTypeMap.getFileExtensionFromUrl(file.toURI().toString());
                if (extension != null) {
                    String type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
                    if (type != null && type.contains("image")) {
                        outputImageFiles.add(file);
                    }
                }
            }
        }
    }

    /**
     * When connection is closed, we stop receiving location updates.
     *
     * @see org.chromium.mojo.bindings.Interface#close()
     */
    @Override
    public void close() {
    }

    /**
     * @see org.chromium.mojo.bindings.Interface#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {
    }
}

/**
 * Android application for running the |CameraRollService|.
 */
public class CameraRollApp implements ApplicationDelegate {
    private final Core mCore;
    private final ExecutorService mThreadPool;

    CameraRollApp(Core core) {
        mCore = core;
        mThreadPool = Executors.newCachedThreadPool();
    }

    /**
     * @see ApplicationDelegate#initialize(Shell, String[], String)
     */

    @Override
    public void initialize(Shell shell, String[] args, String url) {
    }

    /**
     * @see ApplicationDelegate#configureIncomingConnection(ApplicationConnection)
     */
    @Override
    public boolean configureIncomingConnection(ApplicationConnection connection) {
        connection.addService(new ServiceFactoryBinder<CameraRollService>() {
            @Override
            public void bind(InterfaceRequest<CameraRollService> request) {
                CameraRollService.MANAGER.bind(
                        new CameraRollServiceImpl(mCore, mThreadPool), request);
            }

            @Override
            public String getInterfaceName() {
                return CameraRollService.MANAGER.getName();
            }
        });
        return true;
    }

    /**
     * @see ApplicationDelegate#quit()
     */
    @Override
    public void quit() {
        mThreadPool.shutdown();
    }

    public static void mojoMain(
            @SuppressWarnings("unused") Context context, Core core,
            MessagePipeHandle applicationRequestHandle) {
        ApplicationRunner.run(new CameraRollApp(core), core, applicationRequestHandle);
    }
}
