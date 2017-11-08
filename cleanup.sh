#!/bin/bash

make clean

find . -name CMakeFiles -exec rm -rf {} \; &>/dev/null
find . -name MakeFiles -exec rm -rf {} \; &>/dev/null
find . -name cmake_install -exec rm -rf {} \; &>/dev/null
find . -name CMakeCache.txt -exec rm -rf {} \; &>/dev/null
find . -name Makefile -exec rm -rf {} \; &>/dev/null
find . -name cmake_install.cmake -exec rm -rf {} \; &>/dev/null
find . -name *~ -exec rm -rf {} \; &>/dev/null
find . -name install_manifest.txt -exec rm -rf {} \; &>/dev/null
