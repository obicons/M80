#!/bin/bash

pushd dependencies/ELFIO >/dev/null 2>/dev/null
mkdir -p build
cd build
cmake ../
make
popd >/dev/null 2>/dev/null