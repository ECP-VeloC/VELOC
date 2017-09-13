#!/bin/bash
set -x

# note: no need for config.fti (use SCR config instead)
# left the arg there for now

# enable some debug messages
export SCR_DEBUG=1

# number of seconds between checkpoints
export SCR_CHECKPOINT_SECONDS=5

# flush every 10th checkpoint
export SCR_FLUSH=10

#totalview srun -a -n4 -N4 ./hd.exe 4 config.fti
#srun -n4 -N4 ./hd.exe 4 config.fti

#totalview srun -a -n4 -N4 ./hd_mem.exe 4 config.fti
#srun -n4 -N4 ./hd_mem.exe 4 config.fti

#totalview srun -a -n4 -N4 ./hd_file.exe 4 config.fti
#srun -n4 -N4 ./hd_file.exe 4 config.fti

#mpirun -np 8 heatdis_mem 30 config.vec

mpirun -np 8 heatdis_file 30 config.vec

#mpirun -np 8 heatdis_memfile 30 config.vec
