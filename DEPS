# This file is automatically processed to create .DEPS.git which is the file
# that gclient uses under git.
#
# See http://code.google.com/p/chromium/wiki/UsingGit
#
# To test manually, run:
#   python tools/deps2git/deps2git.py -o .DEPS.git -w <gclientdir>
# where <gcliendir> is the absolute path to the directory containing the
# .gclient file (the parent of 'src').
#
# Then commit .DEPS.git locally (gclient doesn't like dirty trees) and run
#   gclient sync
# Verify the thing happened you wanted. Then revert your .DEPS.git change
# DO NOT CHECK IN CHANGES TO .DEPS.git upstream. It will be automatically
# updated by a bot when you modify this one.
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.

vars = {
  'chromium_git': 'https://chromium.googlesource.com',
  'skia_revision': '8cc209111876b7c78b5ec577c9221d8ed5e21024',
  'v8_revision': '3f036fc0ba4ca4483fe77822f8605c277d37ee24',
  'base_revision': '9e74307b276b2f9988005c0e97e85ee222586f79',
  'angle_revision': '46ccef1992a8ede16a596c3dd73cff13c047267d',
  'buildtools_revision': '222bd42ce39d1bd8f08fe089b066f49c469e1cdf',
  'dart_revision': 'f64c907557646699fada4acb7e9a82346a369993',
  'dart_root_certificates_revision': 'aed07942ce98507d2be28cbd29e879525410c7fc',
  'dart_observatory_packages_revision': 'cf90eb9077177d3d6b3fd5e8289477c2385c026a',
  'pdfium_revision': 'ae4256f45df69bbfdf722a6ec17e1e851911ae4e',
  'boringssl_revision': '2deb984187ce8f6c739c780e7fe95e859e93b3da',
  'lss_revision': '6f97298fe3794e92c8c896a6bc06e0b36e4c3de3',
  'nacl_revision': 'f5bc68c59b147b66d553dc6fc17426682cf138e2',
  'tonic_revision': 'f7acabb8fa6c91124486a41194eac3cd421b70d0',
  'gurl_revision': '718eee97ed6a4df41d14726eb2eddc871d9eaaa3',
}

# Only these hosts are allowed for dependencies in this DEPS file.
# If you need to add a new host, contact chrome infrastructure team.
allowed_hosts = [
  'boringssl.googlesource.com',
  'chromium.googlesource.com',
  'pdfium.googlesource.com',
]

deps = {
  'src/base':
    Var('chromium_git') + '/external/github.com/domokit/base' + '@' + Var('base_revision'),

  'src/buildtools':
    Var('chromium_git') + '/chromium/buildtools.git' + '@' + Var('buildtools_revision'),

  'src/testing/gtest':
    Var('chromium_git') + '/external/googletest.git' + '@' + '9855a87157778d39b95eccfb201a9dc90f6d61c6', # from svn revision 746

  'src/testing/gmock':
    Var('chromium_git') + '/external/googlemock.git' + '@' + '0421b6f358139f02e102c9c332ce19a33faf75be', # from svn revision 566

  'src/third_party/angle':
    Var('chromium_git') + '/angle/angle.git' + '@' + Var('angle_revision'),

  'src/third_party/icu':
    Var('chromium_git') + '/chromium/deps/icu.git' + '@' + '94e4b770ce2f6065d4261d29c32683a6099b9d93',

  'src/tools/grit':
    Var('chromium_git') + '/external/grit-i18n.git' + '@' + 'c1b1591a05209c1ad467e845ba8543c22f9072af', # from svn revision 189

  'src/v8':
    Var('chromium_git') + '/v8/v8.git' + '@' + Var('v8_revision'),

  'src/dart':
    Var('chromium_git') + '/external/github.com/dart-lang/sdk.git' + '@' + Var('dart_revision'),

  'src/tonic':
    Var('chromium_git') + '/external/github.com/domokit/tonic' + '@' + Var('tonic_revision'),

  'src/dart/third_party/observatory_pub_packages':
    Var('chromium_git') + '/external/github.com/dart-lang/observatory_pub_packages' + '@' + Var('dart_observatory_packages_revision'),

  'src/dart/third_party/root_certificates':
    Var('chromium_git') + '/external/github.com/dart-lang/root_certificates' + '@' + Var('dart_root_certificates_revision'),

  'src/third_party/skia':
    Var('chromium_git') + '/skia.git' + '@' + Var('skia_revision'),

  'src/third_party/glm':
    Var('chromium_git') + '/external/github.com/g-truc/glm' + '@' + '93d09e0e93ca6fe3d9dc6398489a54a3f9cf76db',

  'src/third_party/yasm/source/patched-yasm':
    Var('chromium_git') + '/chromium/deps/yasm/patched-yasm.git' + '@' + '7da28c6c7c6a1387217352ce02b31754deb54d2a',

  'src/third_party/smhasher/src':
    Var('chromium_git') + '/external/smhasher.git' + '@' + 'e87738e57558e0ec472b2fc3a643b838e5b6e88f',

  'src/third_party/pywebsocket/src':
    Var('chromium_git') + '/external/pywebsocket/src.git' + '@' + 'cb349e87ddb30ff8d1fa1a89be39cec901f4a29c',

  'src/third_party/mesa/src':
    Var('chromium_git') + '/chromium/deps/mesa.git' + '@' + '071d25db04c23821a12a8b260ab9d96a097402f0',

  'src/third_party/pdfium':
    'https://pdfium.googlesource.com/pdfium.git' + '@' + Var('pdfium_revision'),

  'src/third_party/boringssl/src':
    'https://boringssl.googlesource.com/boringssl.git' + '@' + Var('boringssl_revision'),

  'src/third_party/requests/src':
    Var('chromium_git') + '/external/github.com/kennethreitz/requests.git' + '@' + 'f172b30356d821d180fa4ecfa3e71c7274a32de4',

  'src/native_client':
    Var('chromium_git') + '/native_client/src/native_client.git' + '@' + Var('nacl_revision'),

  'src/third_party/pyelftools':
    Var('chromium_git') + '/chromiumos/third_party/pyelftools.git' + '@' + '19b3e610c86fcadb837d252c794cb5e8008826ae',

  'src/third_party/breakpad/src':
    Var('chromium_git') + '/external/google-breakpad/src.git' + '@' + '242fb9a38db6ba534b1f7daa341dd4d79171658b', # from svn revision 1471

  'src/third_party/lss':
    Var('chromium_git') + '/external/linux-syscall-support/lss.git' + '@' + Var('lss_revision'),

  'src/third_party/leveldatabase/src':
    Var('chromium_git') + '/external/leveldb.git' + '@' + '40c17c0b84ac0b791fb434096fd5c05f3819ad55',

  'src/third_party/snappy/src':
    Var('chromium_git') + '/external/snappy.git' + '@' + '762bb32f0c9d2f31ba4958c7c0933d22e80c20bf',

  'src/third_party/ffmpeg':
    Var('chromium_git') + '/chromium/third_party/ffmpeg.git' + '@' + '6f7f37e8c16db3bad5624c7504e710c54bdb7bf5',

  'src/third_party/libcxx/libcxx':
    Var('chromium_git') + '/chromium/llvm-project/libcxx.git' + '@' + '2dcc9a932e33ac4228eedcb6e026ac480daa8e45',

  'src/third_party/libcxx/libcxxabi':
    Var('chromium_git') + '/chromium/llvm-project/libcxxabi.git' + '@' + '3a1fd0deeabcefb42463eb0ac9a570140679e605',

  'src/url':
    Var('chromium_git') + '/external/github.com/domokit/gurl' + '@' + Var('gurl_revision'),
}

deps_os = {
  'android': {
    'src/third_party/colorama/src':
      Var('chromium_git') + '/external/colorama.git' + '@' + '799604a1041e9b3bc5d2789ecbd7e8db2e18e6b8',

    'src/third_party/jsr-305/src':
      Var('chromium_git') + '/external/jsr-305.git' + '@' + '642c508235471f7220af6d5df2d3210e3bfc0919',

    'src/third_party/junit/src':
      Var('chromium_git') + '/external/junit.git' + '@' + '45a44647e7306262162e1346b750c3209019f2e1',

    'src/third_party/mockito/src':
      Var('chromium_git') + '/external/mockito/mockito.git' + '@' + 'ed99a52e94a84bd7c467f2443b475a22fcc6ba8e',

    'src/third_party/robolectric/lib':
      Var('chromium_git') + '/chromium/third_party/robolectric.git' + '@' + '6b63c99a8b6967acdb42cbed0adb067c80efc810',

    'src/third_party/appurify-python/src':
      Var('chromium_git') + '/external/github.com/appurify/appurify-python.git' + '@' + 'ee7abd5c5ae3106f72b2a0b9d2cb55094688e867',

    'src/third_party/freetype-android/src':
      Var('chromium_git') + '/chromium/src/third_party/freetype.git' + '@' + 'd1028db70bea988d1022e4d463de66581c696160',

    'src/third_party/requests/src':
      Var('chromium_git') + '/external/github.com/kennethreitz/requests.git' + '@' + 'f172b30356d821d180fa4ecfa3e71c7274a32de4',

    'src/third_party/pyelftools':
      Var('chromium_git') + '/chromiumos/third_party/pyelftools.git' + '@' + '19b3e610c86fcadb837d252c794cb5e8008826ae',

  },
}


hooks = [
  {
    # This clobbers when necessary (based on get_landmines.py). It must be the
    # first hook so that other things that get/generate into the output
    # directory will not subsequently be clobbered.
    'name': 'landmines',
    'pattern': '.',
    'action': [
        'python',
        'src/build/landmines.py',
    ],
  },
  {
    # Pull clang if needed or requested via GYP_DEFINES.
    'name': 'clang',
    'pattern': '.',
    'action': ['python', 'src/tools/clang/scripts/update.py', '--if-needed'],
  },
  {
    # Pull dart sdk if needed
    'name': 'dart',
    'pattern': '.',
    'action': ['python', 'src/tools/dart/update.py'],
  },
  {
    # This downloads android_tools according to tools/android/VERSION_*.
    'name': 'android_tools',
    'pattern': '.',
    'action': ['python', 'src/tools/android/download_android_tools.py'],
  },
  {
    # This downloads SDK extras and puts them in the
    # third_party/android_tools/sdk/extras directory on the bots. Developers
    # need to manually install these packages and accept the ToS.
    'name': 'sdkextras',
    'pattern': '.',
    # When adding a new sdk extras package to download, add the package
    # directory and zip file to .gitignore in third_party/android_tools.
    'action': ['python', 'src/build/download_sdk_extras.py'],
  },
  # Pull GN binaries. This needs to be before running GYP below.
  {
    'name': 'gn_linux64',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'src/buildtools/linux64/gn.sha1',
    ],
  },
  {
    'name': 'gn_mac',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'src/buildtools/mac/gn.sha1',
    ],
  },
  {
    'name': 'gn_win',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=win*',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'src/buildtools/win/gn.exe.sha1',
    ],
  },
  # Pull clang-format binaries using checked-in hashes.
  {
    'name': 'clang_format_linux',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'src/buildtools/linux64/clang-format.sha1',
    ],
  },
  {
    'name': 'clang_format_mac',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'src/buildtools/mac/clang-format.sha1',
    ],
  },
  # Pull binutils for linux, enabled debug fission for faster linking /
  # debugging when used with clang on Ubuntu Precise.
  # https://code.google.com/p/chromium/issues/detail?id=352046
  {
    'name': 'binutils',
    'pattern': 'src/third_party/binutils',
    'action': [
        'python',
        'src/third_party/binutils/download.py',
    ],
  },
  # Pull eu-strip binaries using checked-in hashes.
  {
    'name': 'eu-strip',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-eu-strip',
                '-s', 'src/build/linux/bin/eu-strip.sha1',
    ],
  },
  # Pull the prebuilt network service binaries according to
  # mojo/public/tools/NETWORK_SERVICE_VERSION.
  {
    'name': 'download_network_service',
    'pattern': '',
    'action': [ 'python',
                'src/mojo/public/tools/download_network_service.py',
                '--tools-directory', '../../../tools',
    ],
  },
  {
    # Ensure that we don't accidentally reference any .pyc files whose
    # corresponding .py files have already been deleted.
    'name': 'remove_stale_pyc_files',
    'pattern': 'src/tools/.*\\.py',
    'action': [
        'python',
        'src/tools/remove_stale_pyc_files.py',
        'src/tools',
    ],
  },
  {
    # This downloads binaries for Native Client's newlib toolchain.
    # Done in lieu of building the toolchain from scratch as it can take
    # anywhere from 30 minutes to 4 hours depending on platform to build.
    'name': 'nacltools',
    'pattern': '.',
    'action': [
        'python', 'src/build/download_nacl_toolchains.py',
        '--packages', 'pnacl_newlib,pnacl_translator',
        'sync', '--extract',
    ],
  },
  {
    # This downloads linux and android Go binaries according to
    # tools/go/VERSION.
    'name': 'gotools',
    'pattern': '.',
    'action': [
        'python', 'src/tools/go/download.py',
    ],
  },
  # Pull DejaVu fonts using checked-in hashes.
  {
    'name': 'dejavu-fonts',
    'pattern': '',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--no_auth',
                '--bucket', 'mojo/dejavu-fonts',
                '-s', 'src/third_party/dejavu-fonts-ttf-2.34/ttf/DejaVuSansMono.ttf.sha1',
    ],
  },
  # Pull keyboard_native resources using checked-in hashes.
  {
    'name': 'keyboard_native_resources',
    'pattern': '',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--no_auth',
                '--bucket', 'mojo/keyboard_native',
                '-d', 'src/services/keyboard_native/res',
    ],
  },
  # Pull dump_syms resources using checked-in hashes.
  {
    'name': 'dump_syms_linux64',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'mojo',
                '-s', 'src/mojo/tools/linux64/dump_syms.sha1',
    ],
  },
  # Pull symupload resources using checked-in hashes.
  {
    'name': 'symupload_linux64',
    'pattern': '.',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'mojo',
                '-s', 'src/mojo/tools/linux64/symupload.sha1',
	],
  },
  # Pull prediction resources using checked-in hashes.
  {
    'name': 'prediction_resources',
    'pattern': '',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--no_auth',
                '--bucket', 'mojo/prediction',
                '-d', 'src/services/prediction/res',
    ],
  },
  # Pull the mojom parser binaries using checked-in hashes.
  {
    'name': 'mojom_tool',
    'pattern': '',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/linux64',
                '-s', 'src/mojo/public/tools/bindings/mojom_tool/bin/linux64/mojom.sha1',
    ],
  },
  {
    'name': 'mojom_tool',
    'pattern': '',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/mac64',
                '-s', 'src/mojo/public/tools/bindings/mojom_tool/bin/mac64/mojom.sha1',
    ],
  },

  # Pull the mojom generator binaries using checked-in hashes.
  {
    'name': 'mojom_generators',
    'pattern': '',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/linux64/generators',
                '-d', 'src/mojo/public/tools/bindings/mojom_tool/bin/linux64/generators',
    ],
  },
  {
    'name': 'mojom_generators',
    'pattern': '',
    'action': [ 'src/tools/download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/mac64/generators',
                '-d', 'src/mojo/public/tools/bindings/mojom_tool/bin/mac64/generators',
    ],
  },
]
