module load cmake/3.18
#module load gcc/8.3
#module load boost
module load gcc/8.1
module load boost/1.72.0
OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$1/../deploy/lib64:$LD_LIBRARY_PATH
mpic++ -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_mem_test $1/test/veloc_chkpt_mem_test.c -lveloc-client
mpic++ -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_file_test $1/test/veloc_chkpt_file_test.c -lveloc-client

mpicc -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_2chkpt_mem_test $1/test/veloc_2chkpt_mem_test.c -lveloc-client
mpicc -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_2chkpt_file_test $1/test/veloc_2chkpt_file_test.c -lveloc-client
