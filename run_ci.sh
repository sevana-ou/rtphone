#!/bin/bash

SEND_MSG="/var/ci/conformance_ci/send_telegram_message.py"

mkdir -p build
cd build

# Configure
cmake ../src
if [ $? -ne 0 ]; then
   /usr/bin/python3 $SEND_MSG "rtphone cmake failed. $BUILD_URL"
   exit 1
fi

# Build
make -j2

if [ $? -ne 0 ]; then
    /usr/bin/python3 $SEND_MSG "rtphone build failed. $BUILD_URL"
    exit 1
fi

/usr/bin/python3 $SEND_MSG "rtphone builds ok. $BUILD_URL"
