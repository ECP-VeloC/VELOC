#!/bin/bash

PREFIX=$1
CFG=heatdis.cfg

export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
rm -rf /tmp/scratch /tmp/persistent

nohup $PREFIX/bin/veloc-backend $CFG > /dev/null 2>&1 &
mpirun -np 2 heatdis_fault 256 $CFG
killall veloc-backend

nohup $PREFIX/bin/veloc-backend $CFG > /dev/null 2>&1 &
mpirun -np 2 heatdis_fault 256 $CFG
EXIT_CODE=$?
killall veloc-backend

exit $EXIT_CODE
