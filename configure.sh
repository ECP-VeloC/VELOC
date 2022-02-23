#!/bin/bash

export CMAKE_PREFIX_PATH=$HOME/deploy/share/cmake
cmake -DCOMM_QUEUE=socket_queue -DWITH_AXL_PREFIX=$HOME/deploy -DWITH_ER_PREFIX=$HOME/deploy -DCMAKE_INSTALL_PREFIX=$HOME/deploy $1
