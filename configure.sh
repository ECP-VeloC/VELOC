#!/bin/bash

export CMAKE_PREFIX_PATH=$HOME/deploy/share/cmake
cmake -DWITH_AXL_PREFIX=$HOME/deploy -DWITH_ER_PREFIX=$HOME/deploy -DBOOST_ROOT=$HOME/deploy -DCMAKE_INSTALL_PREFIX=$HOME/deploy $1
