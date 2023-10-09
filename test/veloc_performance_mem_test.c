#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "include/veloc.h"

int main (int argc, char* argv[])
{
//        char path[256];
	char* path = (char*)malloc(256*sizeof(char));
	path[0] = '\0';
	//path = "/dev/shm/";
//        if (getcwd(path, sizeof(path)) != NULL)
//                printf("current working directory is: %s\n", path);
//        strcat(path,"/");
        strcat(path,"/dev/shm/");
        strcat(path,argv[5]);
	printf("output path IS %s",path);

  int rank, ranks;
  int chkpt_version;
  int core_mem_size_GB = atoi(argv[3]);
  int proc_per_core = atoi(argv[4]);
  //we want to allocate about 1/2 of total node memory between all processes on the node
  //actually, 1/2 is leading to "out of memory" errors. cap at 1/10
  long filesize = (long)(1E9/sizeof(char));
//  long filesize = (long)(core_mem_size_GB*1E9/sizeof(char)/proc_per_core/50);
  //long filesize = (long)(core_mem_size_GB*1E9/sizeof(char)/proc_per_core/10);
  FILE *file_csv_f;
  file_csv_f=fopen(path, "a");
  if(file_csv_f==NULL) {
    perror("Error opening file.");
  }

//  MPI_Init(&argc, &argv);
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided != MPI_THREAD_MULTIPLE) {
      printf("MPI Init failed to provide MPI_THREAD_MULTIPLE");
      exit (2);
  }


  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &ranks);

  if (VELOC_Init(MPI_COMM_WORLD, argv[2]) != VELOC_SUCCESS) {

      printf("Error initializing VELOC! Aborting...\n");

      return 1;
  }
  /* make sure init completes*/
  MPI_Barrier(MPI_COMM_WORLD);

  /* allocate space for the checkpoint data (make filesize a function of rank for some variation) */
  //filesize = filesize + rank;
  char* buf = (char*) malloc(filesize);
  VELOC_Mem_protect(0, buf, filesize, sizeof(char));
  /* time how long it takes to checkpoint */
  MPI_Barrier(MPI_COMM_WORLD);
  double ckpt_start = MPI_Wtime();
  //init_buffer(buf, filesize, rank);
  if(VELOC_Checkpoint("veloc_test", 1) != VELOC_SUCCESS){
    printf("VELOC_Checkpoint FAILED\n");
    return 1;
  }
  double ckpt_local = MPI_Wtime()-ckpt_start;
  double ckpt_finish, ckpt_local_sum, ckpt_finish_max;
  if(VELOC_Checkpoint_wait() == VELOC_SUCCESS){
    ckpt_finish = MPI_Wtime()-ckpt_start;
    MPI_Reduce(&ckpt_local, &ckpt_local_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ckpt_finish, &ckpt_finish_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  }
  else{
    printf("VELOC_Checkpoint FAILED\n");
    return 1;
  }
  if (rank == 0) {
    char buffer[50];
    sprintf(buffer, ",%d,%8.6f,%8.6f\n", proc_per_core, ckpt_local_sum/ranks, ckpt_finish_max);
    fprintf(file_csv_f, buffer);
    fclose(file_csv_f);
printf("buffer=%s\n",buffer);
    printf("computed Checkpoint times are: local Avg %8.6f s\tGlobal Max %8.6f  s\n", ckpt_local_sum/ranks, ckpt_finish_max);
  }

  if(buf != NULL) {
    free(buf);
    buf = NULL;
  }
  VELOC_Finalize(1);
  MPI_Finalize();
  return 0;
}

