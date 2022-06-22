uniq $LSB_DJOB_HOSTFILE | tail -n +2 > nodes_file
nodes=()
while read node; do nodes+=("$node"); done < nodes_file
for i in "${nodes[@]}"; do echo $i; done
#jsrun -r1 ls /dev/shm/scratch/*
rm -rf /dev/shm/scratch/*
pdsh -w ${nodes[2]} rm -rf /dev/shm/scratch/*er*
pdsh -w ${nodes[2]} ls -l /dev/shm/scratch/
#jsrun -r1 ls -l /dev/shm/scratch/*

