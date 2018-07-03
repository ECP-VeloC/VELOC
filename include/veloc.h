#ifndef __VELOC_H
#define __VELOC_H

#include <string.h>
#include <mpi.h>

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Defines */

#ifndef VELOC_SUCCESS
#define VELOC_SUCCESS (0)
#endif

#ifndef VELOC_FAILURE
#define VELOC_FAILURE (-1)
#endif

/** Constants */
static const unsigned int VELOC_VERSION_MAJOR = 0;
static const unsigned int VELOC_VERSION_MINOR = 0;
static const unsigned int VELOC_VERSION_PATH = 0;
static const char VELOC_VERSION[] = "v0.0.0";
static const size_t VELOC_MAX_NAME = 1024;

static const int VELOC_RECOVER_ALL = 0;

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
                            VELOC public functions
---------------------------------------------------------------------------*/

/**************************
 * Init / Finalize
 *************************/

// initializes the VELOC library
//   IN rank      - unique global ID of the process (e.g. MPI rank) 
//   IN cfg_file  - configuration file with the following fields:
//                     scratch = <path> (node-local path where VELOC can save temporary checkpoints that live for the duration of the reservation) 
//                     persistent = <path> (persistent path where VELOC can save durable checkpoints that live indefinitely) 

int VELOC_Init(MPI_Comm comm, const char *cfg_file);
int VELOC_Finalize(int cleanup);
    
/**************************
 * Memory registration
 *************************/
    
// registers a memory region for checkpoint/restart
//   IN id        - application defined integer label for memory region
//   IN ptr       - pointer to start of memory region
//   IN count     - number of consecutive elements in memory region
//   IN base_size - size of each element in memory region
int VELOC_Mem_protect(int id, void *ptr, size_t count, size_t base_size);

// unregisters a memory region
//   IN id        - application defined integer label for memory region
int VELOC_Mem_unprotect(int id);

/**************************
 * File registration
 *************************/
    
// obtain the full path for the file associated with the named checkpoint and version number
// can be used to manually read/write checkpointing data without registering memory regions
int VELOC_Route_file(char *ckpt_file_name);

/**************************
 * Checkpoint routines
 *************************/

// mark start of checkpoint phase
//   IN name - label of the checkpoint
//   IN version - version of the checkpoint, needs to increase with each checkpoint (e.g. iteration number)    
int VELOC_Checkpoint_begin(const char *name, int version);

// write registered memory regions into the checkpoint
// must be called between VELOC_Checkpoint_begin/VELOC_Checkpoint_end
int VELOC_Checkpoint_mem();

// mark end of checkpont phase
//   IN version - version of the checkpoint 
//   IN success - set to 1 if the state restore was successful, 0 otherwise
int VELOC_Checkpoint_end(int success);

// Wait for the checkpoint to complete and return the result (success or failure).
// Only valid in async mode. Typically called before beginning a new checkpoint.
int VELOC_Checkpoint_wait();

int VELOC_Checkpoint(const char *name, int version);
    
/**************************
 * Restart routines
 *************************/

// determine whether application can restart from a previous checkpoint
//   IN name - label of the checkpoint
//   IN needed_version - maximum version to look for
//   returns - VELOC_FAILURE if no checkpoint < needed_version found, latest version < needed_version otherwise
int VELOC_Restart_test(const char *name, int version);

// mark start of restart phase
//   IN name - label of the checkpoint
//   IN version - version of the checkpoint    
int VELOC_Restart_begin(const char *name, int version);

// read registered memory regions from the checkpoint file
// must be called between VELOC_Restart_begin/VELOC_Restart_end
//   IN version - version of the checkpoint
int VELOC_Recover_mem();
        
// mark end of restart phase
//   IN version - version of the checkpoint 
//   IN success - set to 1 if the state restore was successful, 0 otherwise
int VELOC_Restart_end(int success);

int VELOC_Restart(const char *name, int version);
    
#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef __VELOC_H  ----- */
