#!/usr/bin/env bash

if [ ! -d build ]; then
    # Enable testing as this script is only called when building the tests
    mkdir -p build && cd build && cmake -GNinja -DTESTING=1 ..
else
    cd build
fi

ninja $1
