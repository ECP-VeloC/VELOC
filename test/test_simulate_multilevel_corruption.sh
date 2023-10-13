srun -n $SLURM_NNODES hostname > nodes_file
nodes=()
while read node; do nodes+=("$node"); done < nodes_file
for i in "${nodes[@]}"; do echo $i; done
rm -rf /dev/shm/scratch/*
pdsh -w ${nodes[2]} rm -rf /dev/shm/greg_scratch/*er*
pdsh -w ${nodes[2]} ls -l /dev/shm/greg_scratch/

