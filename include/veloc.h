#ifndef _VELOC_H
#define _VELOC_H

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Constants */
static const unsigned int VELOC_VERSION_MAJOR = 0;
static const unsigned int VELOC_VERSION_MINOR = 0;
static const unsigned int VELOC_VERSION_PATH = 0;
static const char VELOC_VERSION[] = "v0.0.0";
static const size_t VELOC_MAX_NAME = 1024;

static const int VELOC_SUCCESS = 0;
static const int VELOC_FAILURE = -1;

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

int VELOC_Init(int rank, const char *cfg_file);
int VELOC_Finalize();
    
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
int VELOC_Route_file(const char *name, int version, char *ckpt_file_name);

/**************************
 * Checkpoint routines
 *************************/

// mark start of checkpoint phase
//   IN name - label of the checkpoint
//   IN version - version of the checkpoint, needs to increase with each checkpoint (e.g. iteration number)    
int VELOC_Checkpoint_begin(const char *name, int version);

// write registered memory regions into the checkpoint
// must be called between VELOC_Checkpoint_begin/VELOC_Checkpoint_end
int VELOC_Checkpoint_mem(int version);

// mark end of checkpont phase
//   IN version - version of the checkpoint 
//   IN success - set to 1 if the state restore was successful, 0 otherwise
int VELOC_Checkpoint_end(int version, int success);

/**************************
 * Restart routines
 *************************/

// determine whether application can restart from a previous checkpoint
//   returns - VELOC_FAILURE if no checkpoint found, latest version otherwise
int VELOC_Restart_test(const char *name);

// mark start of restart phase
//   IN name - label of the checkpoint
//   IN version - version of the checkpoint    
int VELOC_Restart_begin(const char *name, int version);

// read registered memory regions from the checkpoint file
// must be called between VELOC_Restart_begin/VELOC_Restart_end
//   IN version - version of the checkpoint
int VELOC_Restart_mem(int version);
        
// mark end of restart phase
//   IN version - version of the checkpoint 
//   IN success - set to 1 if the state restore was successful, 0 otherwise
int VELOC_Restart_end(int version, int success);

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _VELOC_H  ----- */