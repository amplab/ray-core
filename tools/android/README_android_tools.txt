* How to update Android SDK for Linux/Mac OS X on GCS

1. Run Android SDK Manager and update packages

  $ third_party/android_tools/sdk/tools/android update sdk

2. Choose/Update packages

   The following packages are currently installed:

  - Android SDK Tools 24.4.1
  - Android SDK platform-tools 23.0.1
  - Android SDK Build-tools 23.0.1
  - Android 6.0 (API 23)
    - SDK Platform 23
  - Extras
    - Android Support Library 23.1
    - Google Play services 27

3. Run upload_android_tools.py -t sdk

  $ tools/android/upload_android_tools.py -t sdk

----------------------------------------------------------------------
* How to update Android NDK for Linux/Mac OS X on GCS

1. Download a new NDK binary (e.g. android-ndk-r10e-linux-x86_64.bin)
2. cd third_party/android_tools

  $ cd third_party/android_tools

3. Remove the old ndk directory

  $ rm -rf ndk

4. Run the new NDK binary file

  $ ./android-ndk-r10e-linux-x86_64.bin

5. Rename the extracted directory to ndk

  $ mv android-ndk-r10e ndk

6. Run upload_android_tools.py -t ndk

  $ cd ../..
  $ tools/android/upload_android_tools.py -t ndk
