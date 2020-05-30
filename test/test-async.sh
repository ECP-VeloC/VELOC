#!/bin/bash

PREFIX=$1
BIN=$2
CFG=$(dirname $0)/heatdis.cfg

export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
rm -rf /tmp/scratch /tmp/persistent /tmp/meta
mkdir -p /tmp/meta

nohup $PREFIX/bin/veloc-backend $CFG > /dev/null 2>&1 &
mpirun -np 2 $BIN/heatdis_fault 256 $CFG
killall veloc-backend

nohup $PREFIX/bin/veloc-backend $CFG > /dev/null 2>&1 &
mpirun -np 2 $BIN/heatdis_fault 256 $CFG
EXIT_CODE=$?
killall veloc-backend

exit $EXIT_CODE
