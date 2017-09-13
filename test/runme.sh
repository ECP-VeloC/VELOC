#!/bin/bash

#mpirun -np 8 heatdis_mem 30 config.vec

mpirun -np 8 heatdis_file 30 config.vec

#mpirun -np 8 heatdis_memfile 30 config.vec
