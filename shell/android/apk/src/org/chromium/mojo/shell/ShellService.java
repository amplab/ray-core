// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.shell;

import android.app.Activity;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.ApplicationStatus;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.Pair;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.mojom.mojo.Shell;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

/**
 * A placeholder class to call native functions.
 **/
@JNINamespace("shell")
public class ShellService extends Service {
    private static final String TAG = "ShellService";

    // Name of the intent extra used to hold the application url to start.
    public static final String APPLICATION_URL_EXTRA = "application_url";

    // Directory where applications bundled with the shell will be extracted.
    private static final String LOCAL_APP_DIRECTORY = "local_apps";
    // Individual applications bundled with the shell as assets.
    private static final String NETWORK_LIBRARY_APP = "network_service.mojo";
    // Directory where the child executable will be extracted.
    private static final String CHILD_DIRECTORY = "child";
    // Directory to set TMPDIR to.
    private static final String TMP_DIRECTORY = "tmp";
    // Directory to set HOME to.
    private static final String HOME_DIRECTORY = "home";
    // Name of the child executable.
    private static final String MOJO_SHELL_CHILD_EXECUTABLE = "mojo_shell_child";
    // Path to the default origin of mojo: apps.
    private static final String DEFAULT_ORIGIN = "https://core.mojoapps.io/";
    // Binder to this service.
    private final ShellBinder mBinder = new ShellBinder();
    // A guard flag for calling nativeInit() only once.
    private boolean mInitialized = false;
    // A static reference to the service.
    private static ShellService sShellService;

    private final ThreadLocal<Shell> mShell = new ThreadLocal<Shell>();

    /**
     * Binder for the Shell service. This object is passed to the calling activities.
     **/
    private class ShellBinder extends Binder {
        ShellService getService() {
            return ShellService.this;
        }
    }

    /**
     * Interface implemented by activities wanting to bind with the ShellService service.
     **/
    static interface IShellBindingActivity {
        // Called when the ShellService service is connected.
        void onShellBound(ShellService shellService);

        // Called when the ShellService service is disconnected.
        void onShellUnbound();
    }

    /**
     * ServiceConnection for the ShellService service.
     **/
    static class ShellServiceConnection implements ServiceConnection {
        private final IShellBindingActivity mActivity;

        ShellServiceConnection(IShellBindingActivity activity) {
            this.mActivity = activity;
        }

        @Override
        public void onServiceConnected(ComponentName className, IBinder binder) {
            ShellService.ShellBinder shellBinder = (ShellService.ShellBinder) binder;
            this.mActivity.onShellBound(shellBinder.getService());
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            this.mActivity.onShellUnbound();
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        sShellService = this;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        sShellService = null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // A client is starting this service; make sure the shell is initialized.
        // Note that ensureInitialized is gated by the mInitialized boolean flag. This means that
        // only the first set of arguments will ever be taken into account.
        // TODO(eseidel): ShellService can fail, but we're ignoring the return.
        ensureStarted(getApplicationContext(), getArgsFromIntent(intent));
        if (intent.hasExtra(APPLICATION_URL_EXTRA)) {
            // This intent requests we start an application.
            String urlExtra = intent.getStringExtra(APPLICATION_URL_EXTRA);
            Uri applicationUri = Uri.parse(urlExtra).buildUpon().scheme("https").build();
            startApplicationURL(applicationUri.toString());
        }
        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        if (!mInitialized) {
            throw new IllegalStateException("Start the service first");
        }
        return mBinder;
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        if (!rootIntent.getComponent().getClassName().equals(ViewportActivity.class.getName())) {
            return;
        }
        String viewportId = rootIntent.getStringExtra("ViewportId");
        NativeViewportSupportApplicationDelegate.viewportClosed(viewportId);

        if (ApplicationStatus.getRunningActivities().size() == 0) {
            // There are only background apps in the shell, so we close ourselves so we can cleanly
            // restart. We may want to investigate how to keep backround apps and restart UI
            // activities cleanly.
            nativeQuitShell();
        }
    }

    /**
     * Initializes the native system and starts the shell.
     **/
    private void ensureStarted(Context applicationContext, List<String> args) {
        if (mInitialized) return;
        try {
            FileHelper.extractFromAssets(applicationContext, NETWORK_LIBRARY_APP,
                    getLocalAppsDir(applicationContext), false);
            FileHelper.extractFromAssets(applicationContext, MOJO_SHELL_CHILD_EXECUTABLE,
                    getChildDir(applicationContext), false);
            File mojoShellChild =
                    new File(getChildDir(applicationContext), MOJO_SHELL_CHILD_EXECUTABLE);
            // The shell child executable needs to be ... executable.
            mojoShellChild.setExecutable(true, true);

            List<String> argsList = new ArrayList<String>();

            argsList.add("--origin=" + DEFAULT_ORIGIN);
            argsList.add("--args-for=mojo:notifications " + R.mipmap.ic_launcher);

            File defaultArgumentsFile = new File(
                    String.format("/data/local/tmp/%s.cmd", applicationContext.getPackageName()));
            if (defaultArgumentsFile.isFile()) {
                if (defaultArgumentsFile.canWrite()) {
                    Log.e(TAG, String.format("Command line arguments file (%s) is world writeable. "
                                               + "The file will be ignored.",
                                       defaultArgumentsFile.getAbsolutePath()));
                } else {
                    List<String> defaultArguments = getArgsFromFile(defaultArgumentsFile);
                    if (defaultArguments.size() > 0) {
                        Log.w(TAG, String.format("Adding default arguments from %s:",
                                           defaultArgumentsFile.getAbsolutePath()));
                        for (String arg : defaultArguments) {
                            Log.w(TAG, arg);
                        }
                        argsList.addAll(defaultArguments);
                    }
                }
            }
            if (args != null) {
                argsList.addAll(args);
            }

            nativeStart(applicationContext, applicationContext.getAssets(),
                    mojoShellChild.getAbsolutePath(), argsList.toArray(new String[argsList.size()]),
                    getLocalAppsDir(applicationContext).getAbsolutePath(),
                    getTmpDir(applicationContext).getAbsolutePath(),
                    getHomeDir(applicationContext).getAbsolutePath());
            mInitialized = true;
        } catch (Exception e) {
            Log.e(TAG, "ShellService initialization failed.", e);
            throw new RuntimeException(e);
        }
    }

    private static List<String> getArgsFromFile(File file) {
        List<String> argsList = new ArrayList<String>();
        try (BufferedReader bufferedReader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                argsList.add(line);
            }
            return argsList;
        } catch (IOException e) {
            Log.w(TAG, e.getMessage(), e);
        }
        return new ArrayList<String>();
    }

    private static List<String> getArgsFromIntent(Intent intent) {
        String argsFile = intent.getStringExtra("argsFile");
        if (argsFile != null) {
            File file = new File(argsFile);
            if (!file.isFile()) {
                return null;
            }
            try {
                return getArgsFromFile(file);
            } finally {
                if (!file.delete()) {
                    Log.w(TAG, "Unable to delete args file.");
                }
            }
        }
        return null;
    }

    /**
     * Adds the given URL to the set of mojo applications to run on start. This must be called
     * before {@link ShellService#ensureStarted(Context, List)}
     */
    void addApplicationURL(String url) {
        nativeAddApplicationURL(url);
    }

    /**
     * Starts this application in an already-initialized shell.
     */
    void startApplicationURL(String url) {
        nativeStartApplicationURL(url);
    }

    /**
     * Returns an instance of the shell interface that allows to interact with mojo applications.
     * Can be called on any thread, the returned proxy is thread-local.
     */
    Shell getShell() {
        Shell shell = mShell.get();
        if (shell != null) {
            return shell;
        }
        Pair<Shell.Proxy, InterfaceRequest<Shell>> request =
                Shell.MANAGER.getInterfaceRequest(CoreImpl.getInstance());
        mShell.set(request.first);
        request.first.getProxyHandler().setErrorHandler(new ConnectionErrorHandler() {
            @Override
            public void onConnectionError(MojoException e) {
                mShell.remove();
            }
        });
        nativeBindShell(request.second.passHandle().releaseNativeHandle());
        return mShell.get();
    }

    private static File getLocalAppsDir(Context context) {
        return context.getDir(LOCAL_APP_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getChildDir(Context context) {
        return context.getDir(CHILD_DIRECTORY, Context.MODE_PRIVATE);
    }

    private static File getTmpDir(Context context) {
        return new File(context.getCacheDir(), TMP_DIRECTORY);
    }

    private static File getHomeDir(Context context) {
        return context.getDir(HOME_DIRECTORY, Context.MODE_PRIVATE);
    }

    @CalledByNative
    private static void finishActivities() {
        for (WeakReference<Activity> activityRef : ApplicationStatus.getRunningActivities()) {
            Activity activity = activityRef.get();
            if (activity != null) {
                activity.finishAndRemoveTask();
            }
        }
        if (sShellService != null) {
            sShellService.stopSelf();
        }
    }

    /**
     * Initializes the native system. This API should be called only once per process.
     **/
    private static native void nativeStart(Context context, AssetManager assetManager,
            String mojoShellChildPath, String[] args, String bundledAppsDirectory, String tmpDir,
            String homeDir);

    private static native void nativeAddApplicationURL(String url);

    private static native void nativeStartApplicationURL(String url);

    private static native void nativeBindShell(int shellHandle);

    private static native void nativeQuitShell();
}
