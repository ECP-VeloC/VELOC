#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "include/veloc.h"

/* initialize buffer with some well-known value based on rank */
int init_buffer(char* buf, size_t size, int rank)
{
printf("in init_buffer, SIZE=%d\n", size);
  size_t i;
  for(i=0; i < size; i++) {
    char c = (char) ((size_t)rank + i) % 256;
    buf[i] = c;
  }
  return 0;
}

/* checks buffer for expected value */
int check_buffer(char* buf, size_t size, int rank)
{
printf("in check_buffer, SIZE=%d\n", size);
  size_t i;
  for(i=0; i < size; i++) {
    char c = (char) ((size_t)rank + i) % 256;
    if (buf[i] != c)  {
      return 0;
    }
  }
  return 1;
}

int main (int argc, char* argv[])
{
sleep(3);
  int rank, ranks;
  int chkpt_version;
  size_t filesize = 512*1024;

  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided != MPI_THREAD_MULTIPLE) {
      printf("MPI Init failed to provide MPI_THREAD_MULTIPLE");
      exit (2);
  }


  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &ranks);

  /* time how long it takes to get through init */
  MPI_Barrier(MPI_COMM_WORLD);
  double init_start = MPI_Wtime();
  if (VELOC_Init(MPI_COMM_WORLD, argv[2]) != VELOC_SUCCESS) {

      printf("Error initializing VELOC! Aborting...\n");

      return 1;
  }
printf("CONFIG FILE = %s\n", argv[2]);
  double init_end = MPI_Wtime();
  double secs = init_end - init_start;
  MPI_Barrier(MPI_COMM_WORLD);

  /* compute and print the init stats */
  double secsmin, secsmax, secssum;
  MPI_Reduce(&secs, &secsmin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&secs, &secsmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&secs, &secssum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    printf("Init: Min %8.6f s\tMax %8.6f s\tAvg %8.6f s\n", secsmin, secsmax, secssum/ranks);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  /* allocate space for the checkpoint data (make filesize a function of rank for some variation) */
  filesize = filesize + rank;
  char* buf = (char*) malloc(filesize);
  VELOC_Mem_protect(0, buf, filesize, sizeof(char));

  /* define base name for our checkpoint files */
  char name[256];
  sprintf(name, "rank_%d.ckpt", rank);

//*************************************************
  int v = VELOC_Restart_test("veloc_test", 0);
  printf("VVV in v = VELOC_Restart_test = %d\n",v);
  if(v<-1)v=-1;
  int already_initiated = atoi(argv[3]);
  printf("had already initiated = %d\n",already_initiated);
  if (v >= 0) {
    printf("Previous checkpoint found at iteration %d, initiating restart...\n", v);
    if(VELOC_Restart("veloc_test", v) != VELOC_SUCCESS){
      printf("VELOC_Restart FAILED\n");
      return 1;
    }

    if (!check_buffer(buf, filesize, rank)) {
      printf("%d: Invalid value in buffer\n", rank);
      MPI_Abort(MPI_COMM_WORLD, 1);
      return 1;
    }
    return 0;
  }
  else{
    if(already_initiated != 0){
      printf("Checkpoint had been initiated. Should have been found\n");
      return(1);
    }
    if (rank == 0) {
      printf("No checkpoint to restart from\n");
    }
  }

  init_buffer(buf, filesize, rank);
//first checkpoint
  if(VELOC_Checkpoint("veloc_test", v+1) != VELOC_SUCCESS){
    printf("VELOC_Checkpoint FAILED\n");
    return 1;
  }

//second checkpoint
  if(VELOC_Checkpoint("veloc_test", v+2) != VELOC_SUCCESS){
    printf("VELOC_Checkpoint FAILED\n");
    return 1;
  }

  if(buf != NULL) {
    free(buf);
    buf = NULL;
  }

  VELOC_Finalize(1);
  MPI_Finalize();
  return 0;
}

