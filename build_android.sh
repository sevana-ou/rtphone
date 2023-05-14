#!/bin/bash

rm -rf build_android
mkdir -p build_android
cd build_android

cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake      \
      -DANDROID_NDK=$ANDROID_NDK_HOME                                                   \
      -DANDROID_PLATFORM=24                                                             \
      -DCMAKE_BUILD_TYPE=Release                                                        \
      -DANDROID_ABI="arm64-v8a"                                                       \
      ../src

cmake --build . -j8

