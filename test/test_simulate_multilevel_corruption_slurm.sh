srun -n $SLURM_NNODES hostname > nodes_file
nodes=()
while read node; do nodes+=("$node"); done < nodes_file
for i in "${nodes[@]}"; do echo $i; done
rm -rf /tmp/scratch/*
pdsh -w ${nodes[2]} rm -rf /tmp/scratch/*er*
pdsh -w ${nodes[2]} ls -l /tmp/scratch/

