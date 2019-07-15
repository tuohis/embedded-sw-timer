#!/usr/bin/env bash

if [ ! -d build ]; then
    mkdir -p build && cd build && cmake -GNinja ..
else
    cd build
fi

ninja $1
