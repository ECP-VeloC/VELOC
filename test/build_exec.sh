#module load PrgEnv-gnu/8.1.0
LD_LIBRARY_PATH=$1/../deploy/lib64:$LD_LIBRARY_PATH
mpicxx -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_mem_test $1/test/veloc_chkpt_mem_test.c -lveloc-client
mpicxx -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_file_test $1/test/veloc_chkpt_file_test.c -lveloc-client

mpicxx -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_2chkpt_mem_test $1/test/veloc_2chkpt_mem_test.c -lveloc-client
mpicxx -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_2chkpt_file_test $1/test/veloc_2chkpt_file_test.c -lveloc-client

mpicxx -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_mem_multilevel_corruption_test $1/test/veloc_chkpt_mem_multilevel_corruption_test.c -lveloc-client
mpicxx -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_chkpt_file_multilevel_corruption_test $1/test/veloc_chkpt_file_multilevel_corruption_test.c -lveloc-client


mpicxx -g -DNDEBUG -L$1/../deploy/lib64 -I$1/../deploy/include -I$1 -o $1/build/veloc_performance_mem_test $1/test/veloc_performance_mem_test.c -lveloc-client
