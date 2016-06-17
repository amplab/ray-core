// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.apps.shortcut;

import org.chromium.mojo.system.Core.HandleSignals;
import org.chromium.mojo.system.Core.WaitResult;
import org.chromium.mojo.system.DataPipe;
import org.chromium.mojo.system.DataPipe.ReadFlags;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.MojoResult;
import org.chromium.mojo.system.ResultAnd;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

/**
 * An input stream that reads from a data pipe.
 */
public class DataPipeInputStream extends InputStream {
    /**
     * The handle to read from.
     */
    private final DataPipe.ConsumerHandle mHandle;

    /**
     * Constructor
     *
     * @param handle the handle to read from.
     */
    public DataPipeInputStream(DataPipe.ConsumerHandle handle) {
        mHandle = handle;
    }

    /**
     * @see InputStream#available()
     */
    @Override
    public int available() throws IOException {
        if (!mHandle.isValid()) {
            throw new IOException("Trying to read on a closed handle.");
        }
        ResultAnd<Integer> result = mHandle.readData(null, DataPipe.ReadFlags.none().query(true));
        if (result.getMojoResult() == MojoResult.FAILED_PRECONDITION) {
            return -1;
        }
        if (result.getMojoResult() != MojoResult.OK) {
            throw new IOException(new MojoException(result.getMojoResult()));
        }
        return result.getValue();
    }

    /**
     * @see InputStream#close()
     */
    @Override
    public void close() throws IOException {
        mHandle.close();
    }

    /**
     * @see InputStream#markSupported()
     */
    @Override
    public boolean markSupported() {
        return false;
    }

    /**
     * @see InputStream#read()
     */
    @Override
    public int read() throws IOException {
        if (!waitForReadable()) {
            return -1;
        }
        ByteBuffer buffer = ByteBuffer.allocate(1);
        ResultAnd<Integer> result = mHandle.readData(buffer, ReadFlags.NONE);
        if (result.getMojoResult() != MojoResult.OK) {
            throw new IOException(new MojoException(result.getMojoResult()));
        }
        if (result.getValue() != 1) {
            throw new IOException("Unable to read data.");
        }
        return buffer.get(0);
    }

    /**
     * @see InputStream#read(byte[], int, int)
     */
    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        if (!waitForReadable()) {
            return -1;
        }
        ByteBuffer buffer = mHandle.beginReadData(len, ReadFlags.NONE);
        if (buffer == null) {
            throw new IOException("Unable to read data");
        }
        int nbBytes = buffer.limit();
        if (len < nbBytes) {
            nbBytes = len;
        }
        buffer.get(b, off, nbBytes);
        mHandle.endReadData(nbBytes);
        return nbBytes;
    }

    /**
     * Waits for the handle to be readable. Returns <code>true</code> if the handle is readable,
     * <code>false</code> if it is closed and throws an exception if another error occurs.
     */
    private boolean waitForReadable() throws IOException {
        if (!mHandle.isValid()) {
            throw new IOException("Trying to read on a closed handle.");
        }
        WaitResult wr = mHandle.wait(HandleSignals.READABLE, 0);
        if (wr.getMojoResult() == MojoResult.FAILED_PRECONDITION) {
            return false;
        }
        if (wr.getMojoResult() != MojoResult.OK) {
            throw new IOException(new MojoException(wr.getMojoResult()));
        }
        return true;
    }
}
