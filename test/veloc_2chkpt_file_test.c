#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

//#include "heatdis.h"
#include "include/veloc.h"
// cannot include C++ file 
//#include "src/common/config.hpp"

/* reliable read from file descriptor (retries, if necessary, until hard error) */
ssize_t reliable_read(int fd, void* buf, size_t size)
{
  ssize_t n = 0;
  int retries = 10;
  int rank;
  char host[128];
  while (n < size)
  {
    ssize_t rc = read(fd, (char*) buf + n, size - n);
printf("RC=%d\n",rc);
    if (rc  > 0) { 
      n += rc;
printf("RC>\n");
    } else if (rc == 0) {
      /* EOF */
printf("RC=\n");
      return n;
    } else { /* (rc < 0) */
printf("RC<\n");
      /* something worth printing an error about */
      retries--;
      if (retries) {
        /* print an error and try again */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        gethostname(host, sizeof(host));
      } else {
        /* too many failed retries, give up */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        gethostname(host, sizeof(host));
        printf("ERROR: Giving up checkpoint read\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
      }
    }
  }
  return size;
}

/* reliable write to file descriptor (retries, if necessary, until hard error) */
ssize_t reliable_write(int fd, const void* buf, size_t size)
{
printf("in reliable_write \n");
  ssize_t n = 0;
  int retries = 10;
  int rank;
  char host[128];
  while (n < size)
  {
    ssize_t rc = write(fd, (char*) buf + n, size - n);
printf("RC=%d\n",rc);
    if (rc > 0) {
      n += rc;
    } else if (rc == 0) {
      /* something bad happened, print an error and abort */
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      gethostname(host, sizeof(host));
      printf("%d on %s: ERROR: Error writing: write(%d, %p, %ld) returned 0 @ %s:%d\n",
              rank, host, fd, (char*) buf + n, size - n, __FILE__, __LINE__
      );
      MPI_Abort(MPI_COMM_WORLD, 0);
    } else { /* (rc < 0) */
      /* something worth printing an error about */
      retries--;
      if (retries) {
        /* print an error and try again */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        gethostname(host, sizeof(host));
      } else {
        /* too many failed retries, give up */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        gethostname(host, sizeof(host));
        printf("ERROR: Giving up checkpoint write\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
      }
    }
  }
  return size;
}

/* read the checkpoint data from file into buf, and return whether the read was successful */
int read_checkpoint(char* file, int* ckpt, char* buf, size_t size)
{
printf("in read_checkpoint\n");
  ssize_t n;
  char ckpt_buf[7];

  int fd = open(file, O_RDONLY);
//int fd = -2;
  if (fd > 0) {
    /* read the checkpoint id */
printf("before read chepoint id\n");
    n = reliable_read(fd, ckpt_buf, sizeof(ckpt_buf));

    /* read the checkpoint data, and check the file size */
printf("before read chepoint data\n");
    n = reliable_read(fd, buf, size);
printf("after read chepoint data\n");
    if (n != size) {
      printf("Filesize not correct. Expected %lu, got %lu\n", size, n);
      close(fd);
      return 0;
    }

    /* if the file looks good, set the timestep and return */
    (*ckpt) = atoi(ckpt_buf);

    close(fd);

    return 1;
  }
  else {
  	printf("Could not open file %s\n", file);
  }

  return 0;
}

/* write the checkpoint data to fd, and return whether the write was successful */
int write_checkpoint(int fd, int ckpt, char* buf, size_t size)
{
printf("in write_checkpoint \n");
  ssize_t rc;

  /* write the checkpoint id (application timestep) */
  char ckpt_buf[7];
  sprintf(ckpt_buf, "%06d", ckpt);
  rc = reliable_write(fd, ckpt_buf, sizeof(ckpt_buf));
printf("in write_checkpoint aftre first write \n");
  if (rc < 0) return 0;

  /* write the checkpoint data */
  rc = reliable_write(fd, buf, size);
printf("in write_checkpoint aftre second write \n");
  if (rc < 0) return 0;

  return 1;
}

/* initialize buffer with some well-known value based on rank */
int init_buffer(char* buf, size_t size, int rank, int ckpt)
{
  size_t i;
  for(i=0; i < size; i++) {
    /*char c = 'a' + (rank+i) % 26;*/
    char c = (char) ((size_t)rank + i) % 256;
    buf[i] = c;
  }
  return 0;
}

/* checks buffer for expected value */
int check_buffer(char* buf, size_t size, int rank, int ckpt)
{
printf("in check_buffer, SIZE=%d\n", size);
  size_t i;
  for(i=0; i < size; i++) {
    /*char c = 'a' + (rank+i) % 26;*/
    char c = (char) ((size_t)rank + i) % 256;
//printf("i=%d, c=%c, buf_i=%c\n", i,c,buf[i]);
    if (buf[i] != c)  {
      return 0;
    }
  }
  return 1;
}

int main (int argc, char* argv[])
{
  int rank, ranks;
  int chkpt_version;
  size_t filesize = 512*1024;
  int  timestep = 0;

//  MPI_Init(&argc, &argv);
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

  /* define base name for our checkpoint files */
  char name[256];
  sprintf(name, "rank_%d.ckpt", rank);

//*************************************************
int already_initiated = atoi(argv[3]);
printf("had already initiated = %d\n",already_initiated);
int v;
if(already_initiated != 0){ 
  v = VELOC_Restart_test("veloc_test", 0);
  printf("VVV in v = VELOC_Restart_test = %d\n",v);
//v=-1;
  if (v >= 0) {
    printf("Previous checkpoint found at iteration %d, initiating restart...\n", v);
    if(VELOC_Restart_begin("veloc_test", v) != VELOC_SUCCESS){
      printf("VELOC_Restart_begin FAILED\n");
      return 1;
    }
    char original[VELOC_MAX_NAME], veloc_file[VELOC_MAX_NAME];
    sprintf(original, "veloc_test-file-ckpt_%d_%d.dat", v, rank);
    printf("ORIGINAL=%s\n", original);
    if(VELOC_Route_file(original, veloc_file) != VELOC_SUCCESS){
      printf("VELOC_Route_file FAILED\n");
      return 1;
    }
    printf("vvveloc_file is %s\n",veloc_file);
    /* read the data */
    int valid = 1;
    if (read_checkpoint(veloc_file, &timestep, buf, filesize)){
      /* read the file ok, now check that contents are good */
      if (!check_buffer(buf, filesize, rank, timestep)) {
        printf("%d: Invalid value in buffer\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
      }
    } else {
          valid = 0;
    	  printf("%d: Could not read checkpoint %d from %s\n", rank, timestep, veloc_file);
    }
    if(VELOC_Restart_end(valid) != VELOC_SUCCESS){
      printf("VELOC_Restart_end FAILED\n");
      return 1;
    }
    return 0;
  } 
  else{
    if (rank == 0) {
      printf("No checkpoint to restart from\n");
    }
  }
}
else{
//write first checkpoint
  if(VELOC_Checkpoint_begin("veloc_test", v+1) != VELOC_SUCCESS){
    printf("VELOC_Checkpoint_begin FAILED\n");
    return 1;
  }
  char original[VELOC_MAX_NAME], veloc_file[VELOC_MAX_NAME];
  sprintf(original, "veloc_test-file-ckpt_%d_%d.dat", v+1, rank);
  printf("ORIGINAL=%s\n", original);
  if(VELOC_Route_file(original, veloc_file) != VELOC_SUCCESS){
    printf("VELOC_Route_file FAILED\n");
    return 1;
  }
  printf("vvveloc_file is %s\n",veloc_file);
  int valid = 1;
  init_buffer(buf, filesize, rank, timestep);
  timestep++;
  printf("trying to open file %s\n", veloc_file);
  char com2[50];
  sprintf(com2, "ls -l %s", veloc_file);
  system(com2);
  int fd = open(veloc_file, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd < 0) {
    perror("errror with open");
    exit(1);
  }
  printf("printing file permissionsi\n");
  char com1[50];
  sprintf(com1, "ls -l %s", veloc_file);
  system(com1);
  printf("FD=%d, timestamp=%d, filesize=%d\n", fd,timestep,filesize);
  if(!write_checkpoint(fd, timestep, buf, filesize)){
    valid = 0;
  }
  if(VELOC_Checkpoint_end(valid) != VELOC_SUCCESS){
    printf("VELOC_Checkpoint_end FAILED\n");
    return 1;
  }
  assert(VELOC_Checkpoint_wait() == VELOC_SUCCESS);

//write second checkpoint
  if(VELOC_Checkpoint_begin("veloc_test", v+2) != VELOC_SUCCESS){
    printf("VELOC_Checkpoint_begin FAILED\n");
    return 1;
  }
  char original1[VELOC_MAX_NAME], veloc_file1[VELOC_MAX_NAME];
  sprintf(original1, "veloc_test-file-ckpt_%d_%d.dat", v+2, rank);
  printf("ORIGINAL=%s\n", original1);
  if(VELOC_Route_file(original1, veloc_file1) != VELOC_SUCCESS){
    printf("VELOC_Route_file FAILED\n");
    return 1;
  }
  printf("vvveloc_file1 is %s\n",veloc_file1);
  int valid1 = 1;
  init_buffer(buf, filesize, rank, timestep);
  timestep++;
  printf("trying to open file %s\n", veloc_file1);
  char com12[50];
  sprintf(com12, "ls -l %s", veloc_file1);
  system(com12);
  int fd1 = open(veloc_file1, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd1 < 0) {
    perror("errror with open");
    exit(1);
  }
  printf("printing file permissionsi\n");
  char com11[50];
  sprintf(com11, "ls -l %s", veloc_file1);
  system(com11);
  printf("FD=%d, timestamp=%d, filesize=%d\n", fd1,timestep,filesize);
  if(!write_checkpoint(fd1, timestep, buf, filesize)){
    valid1 = 0;
  }
  if(VELOC_Checkpoint_end(valid1) != VELOC_SUCCESS){
    printf("VELOC_Checkpoint_end FAILED\n");
    return 1;
  }

  if(buf != NULL) {
    free(buf);
    buf = NULL;
  }
  assert(VELOC_Checkpoint_wait() == VELOC_SUCCESS);
//VELOC_Checkpoint_wait();
}
  VELOC_Finalize(1);
  MPI_Finalize();
  return 0;
}
