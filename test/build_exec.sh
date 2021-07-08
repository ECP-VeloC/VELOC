module load cmake/3.18
module load gcc/8.3
module load boost
OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
LD_LIBRARY_PATH=$1/../deploy/lib64:$LD_LIBRARY_PATH
mpicc -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_mem_test $1/test/veloc_chkpt_mem_test.c -lveloc-client
mpicc -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_file_test $1/test/veloc_chkpt_file_test.c -lveloc-client

