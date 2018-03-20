#!/bin/bash

PREFIX=$HOME/deploy
V_MAJOR=1
V_MINOR=66
V_ADDITIONAL=0

VERSION=${V_MAJOR}_${V_MINOR}_${V_ADDITIONAL}

cd /tmp
wget https://dl.bintray.com/boostorg/release/${V_MAJOR}.${V_MINOR}.${V_ADDITIONAL}/source/boost_${VERSION}.tar.bz2
tar xjf boost_${VERSION}.tar.bz2
cd boost_$VERSION
mkdir -p $PREFIX
./bootstrap.sh --prefix=$PREFIX --with-libraries=system,mpi,serialization,filesystem
echo "using mpi ;" >> project-config.jam
./b2 variant=release link=shared threading=multi runtime-link=shared --prefix=$PREFIX install
