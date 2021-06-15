#!/bin/bash
nohup jsrun -r 1 ../../../deploy/bin/veloc-backend veloc_lsf.cfg >output 2>&1 &
sleep 5
exit 0
