// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import org.chromium.mojo.bindings.DeserializationException;
import org.chromium.mojo.bindings.Message;
import org.chromium.mojo.system.Handle;
import org.chromium.mojom.nfc.Db;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * Manages access to the Nfc Db.
 */
class NfcDbManager {
    private static class LazyHolder {
        private static final NfcDbManager INSTANCE = new NfcDbManager();
    }

    /**
     * Returns the instance.
     */
    public static NfcDbManager getInstance() {
        return LazyHolder.INSTANCE;
    }

    // The current version of the database. This must be incremented each time Db definition in
    // nfc_impl_db.mojom is changed in a non backward-compatible way.
    private static final int VERSION = 0;

    private Db mDb = null;
    private File mDbFile = null;

    private File getDbFile() {
        if (mDbFile != null) {
            return mDbFile;
        }
        File home = new File(System.getenv("HOME"));
        File configDir = new File(home, ".mojo_nfc");
        configDir.mkdirs();
        mDbFile = new File(configDir, "db");
        return mDbFile;
    }

    Db getDb() {
        if (mDb != null) {
            return mDb;
        }
        File dbFile = getDbFile();
        if (dbFile.exists()) {
            try {
                int size = (int) dbFile.length();
                try (FileInputStream stream = new FileInputStream(dbFile);
                        FileChannel channel = stream.getChannel()) {
                    // Use mojo serialization to read the database.
                    Db db = Db.deserialize(new Message(
                            channel.map(MapMode.READ_ONLY, 0, size), new ArrayList<Handle>()));
                    if (db.version == VERSION) {
                        mDb = db;
                        return mDb;
                    }
                } catch (DeserializationException e) {
                }
                dbFile.delete();
            } catch (IOException e) {
            }
        }
        mDb = new Db();
        mDb.version = VERSION;
        mDb.registeredApplications = new String[0];
        return mDb;
    }

    void addRegisteredApplication(String applicationUrl) {
        Db db = getDb();
        ArrayList<String> registeredApplications =
                new ArrayList<String>(Arrays.asList(db.registeredApplications));
        if (registeredApplications.contains(applicationUrl)) {
            return;
        }
        registeredApplications.add(applicationUrl);
        db.registeredApplications =
                registeredApplications.toArray(new String[registeredApplications.size()]);

        try {
            // Use mojo serialization to persist the database.
            Message m = db.serialize(null);
            File dbFile = getDbFile();
            dbFile.delete();
            try (FileOutputStream stream = new FileOutputStream(dbFile);
                    FileChannel channel = stream.getChannel()) {
                channel.write(m.getData());
            }
        } catch (IOException e) {
        }
    }

    void removeRegisteredApplication(String applicationUrl) {
        Db db = getDb();
        ArrayList<String> registeredApplications =
                new ArrayList<String>(Arrays.asList(db.registeredApplications));
        if (!registeredApplications.contains(applicationUrl)) {
            return;
        }
        registeredApplications.remove(registeredApplications);
        db.registeredApplications =
                registeredApplications.toArray(new String[registeredApplications.size()]);

        try {
            // Use mojo serialization to persist the database.
            Message m = db.serialize(null);
            File dbFile = getDbFile();
            dbFile.delete();
            try (FileOutputStream stream = new FileOutputStream(dbFile);
                    FileChannel channel = stream.getChannel()) {
                channel.write(m.getData());
            }
        } catch (IOException e) {
        }
    }

    String[] getRegisteredApplications() {
        return getDb().registeredApplications;
    }
}
