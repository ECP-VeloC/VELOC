#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <build_dir>"
    exit 1
fi

INSTALL_PREFIX=$HOME/deploy
export CMAKE_PREFIX_PATH=$INSTALL_PREFIX/share/cmake:$INSTALL_PREFIX/lib/cmake
cmake -DCOMM_QUEUE=socket_queue -DPOSIX_IO=direct -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX $1
