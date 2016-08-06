#!/bin/bash

set -x
set -e

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE:-$0}")"; pwd)

mkdir -p $ROOT_DIR/third_party/

# Get gsutil.
cd $ROOT_DIR/third_party/
wget https://storage.googleapis.com/pub/gsutil.tar.gz
tar xfz gsutil.tar.gz
rm gsutil.tar.gz

# Get gn.
$ROOT_DIR/third_party/gsutil/gsutil cp gs://chromium-gn/026de1fbd84dd21d341437a80bb2e354e8b1f8cb $ROOT_DIR/third_party/gn
chmod 700 $ROOT_DIR/third_party/gn

# Get and build ninja.
cd $ROOT_DIR/third_party/
if [ ! -d ninja-1.5.1 ]; then
  git clone https://github.com/martine/ninja.git ninja-1.5.1 -b v1.5.1
  ./ninja-1.5.1/bootstrap.py
  cp ./ninja-1.5.1/ninja $ROOT_DIR/third_party/
  chmod 700 $ROOT_DIR/third_party/ninja
fi

cd $ROOT_DIR/src
if [ ! -d base ]; then
  git clone https://github.com/domokit/base
  cd base
  git checkout 9e74307b276b2f9988005c0e97e85ee222586f79
fi

cd $ROOT_DIR/src
if [ ! -d url ]; then
  git clone https://github.com/domokit/gurl url
  cd url
  git checkout 718eee97ed6a4df41d14726eb2eddc871d9eaaa3
fi

cd $ROOT_DIR/src/third_party
if [ ! -d icu ]; then
  git clone https://chromium.googlesource.com/chromium/deps/icu.git
  cd icu
  git checkout 94e4b770ce2f6065d4261d29c32683a6099b9d93
fi

cd $ROOT_DIR/src/third_party
if [ ! -d googletest ]; then
	git clone https://chromium.googlesource.com/external/github.com/google/googletest/
  cd googletest
  git checkout ec44c6c1675c25b9827aacd08c02433cccde7780
  cp -r googletest $ROOT_DIR/src/testing/gtest
  cp -r googlemock $ROOT_DIR/src/testing/gmock
fi

cd $ROOT_DIR/src/third_party/boringssl
if [ ! -d src ]; then
  git clone https://boringssl.googlesource.com/boringssl.git src
  cd src
  git checkout 2deb984187ce8f6c739c780e7fe95e859e93b3da
fi

cd $ROOT_DIR/src/third_party/
if [ ! -d leveldatabase ]; then
  mkdir leveldatabase
  cd leveldatabase
  git clone https://chromium.googlesource.com/external/leveldb.git src
  cd src
  git checkout 40c17c0b84ac0b791fb434096fd5c05f3819ad55
fi

cd $ROOT_DIR/src/third_party/snappy
if [ ! -d src ]; then
  git clone https://chromium.googlesource.com/external/snappy.git src
  cd src
  git checkout 762bb32f0c9d2f31ba4958c7c0933d22e80c20bf
fi

cd $ROOT_DIR/src/third_party/breakpad
if [ ! -d src ]; then
  git clone https://chromium.googlesource.com/external/google-breakpad/src.git
  cd src
  git checkout 242fb9a38db6ba534b1f7daa341dd4d79171658b
fi

cd $ROOT_DIR/src/third_party/
if [ ! -d lss ]; then
  git clone https://chromium.googlesource.com/external/linux-syscall-support/lss.git
  cd lss
  git checkout 6f97298fe3794e92c8c896a6bc06e0b36e4c3de3
fi

cd $ROOT_DIR/src/third_party/
if [ ! -d ray-artifacts ]; then
  git clone https://github.com/amplab/ray-artifacts
  cd ray-artifacts
  mkdir -p $ROOT_DIR/src/out/Debug
  cp -r gen $ROOT_DIR/src/out/Debug
  cp -r clang_x86 $ROOT_DIR/src/out/Debug
fi
