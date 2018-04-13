#!/bin/bash

cmake -DWITH_AXL_PREFIX=$HOME/work/veloc/deps/install -DWITH_ER_PREFIX=$HOME/work/veloc/deps/install -DBOOST_ROOT=$HOME/deploy -DCMAKE_INSTALL_PREFIX=$HOME/deploy
