// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.services.contacts;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.util.Base64;
import android.util.Log;

import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.RunLoop;
import org.chromium.mojom.contacts.Contact;
import org.chromium.mojom.contacts.ContactsService;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * Implementation of {@link ContactsService}.
 */
public class ContactsServiceImpl implements ContactsService {
    private static final String TAG = "ContactsServiceImpl";

    // Columns to retrieve from the contact table.
    private static final String[] CONTACT_PROJECTION = {
            ContactsContract.Contacts._ID, ContactsContract.Contacts.DISPLAY_NAME_PRIMARY,
    };

    private static final String CONTACT_SELECTION_CLAUSE = String.format(
            "%s LIKE '%%' || ? || '%%'", ContactsContract.Contacts.DISPLAY_NAME_PRIMARY);
    private static final String CONTACT_ORDER_BY =
            String.format("%s COLLATE NOCASE COLLATE LOCALIZED ASC",
                    ContactsContract.Contacts.DISPLAY_NAME_PRIMARY);
    private static final String CONTACT_ORDER_BY_AND_LIMIT = CONTACT_ORDER_BY + " LIMIT %d,%d";

    // Columns to retrieve from the email table.
    private static final String[] EMAIL_PROJECTION = {ContactsContract.CommonDataKinds.Email.DATA};

    private static final String EMAIL_SELECTION_CLAUSE =
            String.format("%s = ?", ContactsContract.CommonDataKinds.Email.CONTACT_ID);
    private static final String EMAIL_ORDER_BY = String.format(
            "%s COLLATE NOCASE COLLATE LOCALIZED ASC", ContactsContract.CommonDataKinds.Email.DATA);

    private final Executor mExecutor = Executors.newCachedThreadPool();
    private final Context mContext;
    private final Core mCore;

    public ContactsServiceImpl(Context context, Core core) {
        mContext = context;
        mCore = core;
    }

    private static byte[] streamToByte(InputStream input) {
        try (InputStream is = input; ByteArrayOutputStream buffer = new ByteArrayOutputStream();) {
            int nRead;
            byte[] data = new byte[16384];
            while ((nRead = is.read(data, 0, data.length)) != -1) {
                buffer.write(data, 0, nRead);
            }
            buffer.flush();
            return buffer.toByteArray();
        } catch (IOException e) {
            Log.w(TAG, "Unable to convert stream: " + e.getMessage(), e);
            return null;
        }
    }

    /**
     * @see ContactsService#getCount(java.lang.String, ContactsService.GetCountResponse)
     */
    @Override
    public void getCount(final String filter, final GetCountResponse callback) {
        final RunLoop runLoop = mCore.getCurrentRunLoop();
        mExecutor.execute(new Runnable() {

            @Override
            public void run() {
                long count = 0;
                ContentResolver cr = mContext.getContentResolver();
                try (Cursor cursor = cr.query(ContactsContract.Contacts.CONTENT_URI,
                             CONTACT_PROJECTION, CONTACT_SELECTION_CLAUSE,
                             new String[] {filter == null ? "" : filter}, CONTACT_ORDER_BY)) {
                    count = cursor.getCount();
                } finally {
                    final long finalCount = count;
                    runLoop.postDelayedTask(new Runnable() {
                        @Override
                        public void run() {
                            callback.call(finalCount);
                        }
                    }, 0);
                }
            }
        });
    }

    /**
     * @see ContactsService#get(String, int, int, GetResponse)
     */
    @Override
    public void get(
            final String filter, final int offset, final int limit, final GetResponse callback) {
        final RunLoop runLoop = mCore.getCurrentRunLoop();
        mExecutor.execute(new Runnable() {

            @Override
            public void run() {
                final List<Contact> contacts = new ArrayList<Contact>();
                ContentResolver cr = mContext.getContentResolver();
                try (Cursor cursor = cr.query(ContactsContract.Contacts.CONTENT_URI,
                             CONTACT_PROJECTION, CONTACT_SELECTION_CLAUSE,
                             new String[] {filter == null ? "" : filter},
                             String.format(CONTACT_ORDER_BY_AND_LIMIT, offset, limit))) {
                    while (cursor.moveToNext()) {
                        int id =
                                cursor.getInt(cursor.getColumnIndex(ContactsContract.Contacts._ID));
                        String name = cursor.getString(cursor.getColumnIndex(
                                ContactsContract.Contacts.DISPLAY_NAME_PRIMARY));
                        Contact contact = new Contact();
                        contact.id = id;
                        contact.name = name;
                        contacts.add(contact);
                    }
                } finally {
                    runLoop.postDelayedTask(new Runnable() {
                        @Override
                        public void run() {
                            callback.call(contacts.toArray(new Contact[contacts.size()]));
                        }
                    }, 0);
                }
            }
        });
    }

    /**
     * @see ContactsService#getEmails(long, ContactsService.GetEmailsResponse)
     */
    @Override
    public void getEmails(final long id, final GetEmailsResponse callback) {
        final RunLoop runLoop = mCore.getCurrentRunLoop();
        mExecutor.execute(new Runnable() {

            @Override
            public void run() {
                final List<String> emails = new ArrayList<String>();
                ContentResolver cr = mContext.getContentResolver();
                try (Cursor cursor = cr.query(ContactsContract.CommonDataKinds.Email.CONTENT_URI,
                             EMAIL_PROJECTION, EMAIL_SELECTION_CLAUSE,
                             new String[] {String.valueOf(id)}, EMAIL_ORDER_BY)) {
                    while (cursor.moveToNext()) {
                        emails.add(cursor.getString(cursor.getColumnIndex(
                                ContactsContract.CommonDataKinds.Email.DATA)));
                    }
                } finally {
                    runLoop.postDelayedTask(new Runnable() {
                        @Override
                        public void run() {
                            callback.call(emails.toArray(new String[emails.size()]));
                        }
                    }, 0);
                }
            }
        });
    }

    /**
     * @see ContactsService#getPhoto(long, boolean, ContactsService.GetPhotoResponse)
     */
    @Override
    public void getPhoto(
            final long id, final boolean highResolution, final GetPhotoResponse callback) {
        final RunLoop runLoop = mCore.getCurrentRunLoop();
        mExecutor.execute(new Runnable() {

            @Override
            public void run() {
                String photoDataURL = null;
                try {
                    Uri contactURI = ContentUris.withAppendedId(Contacts.CONTENT_URI, id);
                    InputStream photoDataStream = Contacts.openContactPhotoInputStream(
                            mContext.getContentResolver(), contactURI, highResolution);
                    if (photoDataStream != null) {
                        byte[] bytes = streamToByte(photoDataStream);
                        if (bytes != null) {
                            String base64 = Base64.encodeToString(bytes, Base64.DEFAULT);
                            photoDataURL = "data:image/jpeg;base64," + base64;
                        }
                    }
                } finally {
                    final String finalPhotoDataURL = photoDataURL;
                    runLoop.postDelayedTask(new Runnable() {
                        @Override
                        public void run() {
                            callback.call(finalPhotoDataURL);
                        }
                    }, 0);
                }
            }
        });
    }

    /**
     * @see org.chromium.mojo.bindings.Interface#close()
     */
    @Override
    public void close() {}

    /**
     * @see org.chromium.mojo.bindings.ConnectionErrorHandler#onConnectionError(MojoException)
     */
    @Override
    public void onConnectionError(MojoException e) {}
}
