srun -n $SLURM_NNODES hostname > nodes_file
nodes=()
while read node; do nodes+=("$node"); done < nodes_file
for i in "${nodes[@]}"; do echo $i; done
echo "files on node 0\n"
pdsh -w ${nodes[0]} ls -l /tmp/scratch/
echo "files on node 1\n"
pdsh -w ${nodes[1]} ls -l /tmp/scratch/
echo "files on node 2\n"
pdsh -w ${nodes[2]} ls -l /tmp/scratch/
echo "files on node 3\n"
pdsh -w ${nodes[3]} ls -l /tmp/scratch/

