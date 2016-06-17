// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:io';

import 'package:mojom/src/command_runner.dart';
import 'package:mojom/src/utils.dart';
import 'package:path/path.dart' as path;
import 'package:unittest/unittest.dart';

final String singlePacakgeMojomContents = '''
[DartPackage="single_package"]
module single_package;
struct SinglePackage {
  int32 thingo;
};
''';

final String singlePackagePubspecContents = '''
name: single_package
''';

Future runCommand(List<String> args) {
  return new MojomCommandRunner().run(args);
}

Future<String> setupPackage(
    String basePath, String dirName, String packageName, String pubspec) async {
    // //dirName
    String packagePath = path.join(basePath, dirName);
    // //dirName/lib
    String packageLibPath = path.join(packagePath, 'lib');
    // //dirName/packages
    String packagePackagesPath = path.join(packagePath, 'packages');
    // //dirName/packages/packageName
    String packagePackagePath =
        path.join(packagePackagesPath, packageName);
    // //dirName/pubspec.yaml
    String pubspecPath = path.join(packagePath, 'pubspec.yaml');

    // Create the directory structure
    await new Directory(packageLibPath).create(recursive: true);
    await new Directory(packagePackagesPath).create(recursive: true);
    await new Link(packagePackagePath).create(packageLibPath);

    // Write the pubspec.yaml
    File pubspecFile = new File(pubspecPath);
    await pubspecFile.create(recursive: true);
    await pubspecFile.writeAsString(pubspec);

    return packagePath;
}

Future destroyPackage(String basePath, String dirName) async {
  await new Directory(path.join(basePath, dirName)).delete(recursive: true);
}

main() async {
  String mojoSdk;
  if (Platform.environment['MOJO_SDK'] != null) {
    mojoSdk = Platform.environment['MOJO_SDK'];
  } else {
    mojoSdk = path.normalize(path.join(
        path.dirname(Platform.script.path), '..', '..', '..', '..', 'public'));
  }
  if (!await new Directory(mojoSdk).exists()) {
    fail("Could not find the Mojo SDK");
  }

  final scriptPath = path.dirname(Platform.script.path);

  // //test_mojoms/mojom
  final testMojomPath = path.join(scriptPath, 'test_mojoms');

  setUp(() async {
    await new Directory(testMojomPath).create(recursive: true);

    // //test_mojoms/single_package/public/interfaces/single_package.mojom
    final singlePackageMojomFile = new File(path.join(testMojomPath,
        'single_package', 'public', 'interfaces', 'single_package.mojom'));
    await singlePackageMojomFile.create(recursive: true);
    await singlePackageMojomFile.writeAsString(singlePacakgeMojomContents);
  });

  tearDown(() async {
    await new Directory(testMojomPath).delete(recursive: true);
  });

  group('Commands', () {
    test('single', () async {
      String packagePath = await setupPackage(
          scriptPath, 'single_package', 'single_package',
          singlePackagePubspecContents);

      await runCommand([
        'single',
        '-m',
        mojoSdk,
        '-r',
        testMojomPath,
        '-p',
        packagePath
      ]);

      // Should have:
      // //single_package/lib/single_package/single_package.mojom.dart
      final resultPath = path.join(
          packagePath, 'lib', 'single_package', 'single_package.mojom.dart');
      final resultFile = new File(resultPath);
      expect(await resultFile.exists(), isTrue);

      await destroyPackage(scriptPath, 'single_package');
    });

    test('gen', () async {
      String packagePath = await setupPackage(
          scriptPath, 'single_package', 'single_package',
          singlePackagePubspecContents);

      await runCommand(
          ['gen', '-m', mojoSdk, '-r', testMojomPath, '-o', scriptPath]);

      // Should have:
      // //single_package/lib/single_package/single_package.mojom.dart
      final resultPath = path.join(
          packagePath, 'lib', 'single_package', 'single_package.mojom.dart');
      final resultFile = new File(resultPath);
      expect(await resultFile.exists(), isTrue);

      await destroyPackage(scriptPath, 'single_package');
    });

    test('gen wrong name', () async {
      String packagePath = await setupPackage(
          scriptPath, 'wrong_name', 'single_package',
          singlePackagePubspecContents);

      await runCommand(
          ['gen', '-m', mojoSdk, '-r', testMojomPath, '-o', scriptPath]);

      final resultPath = path.join(
          packagePath, 'lib', 'single_package', 'single_package.mojom.dart');
      final resultFile = new File(resultPath);
      expect(await resultFile.exists(), isTrue);

      await destroyPackage(scriptPath, 'wrong_name');
    });
  });
}
