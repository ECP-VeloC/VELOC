#ifndef _VELOC_H
#define _VELOC_H

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Compile-time macros to define API version */
#define VELOC_VERSION_MAJOR (0)
#define VELOC_VERSION_MINOR (0)
#define VELOC_VERSION_PATCH (0)
#define VELOC_VERSION "v0.0.0"

/** Token returned if a VEOC function succeeds.                             */
#define VELOC_SUCCESS (0)
#define VELOC_FAILURE (1)

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
                            VELOC types
---------------------------------------------------------------------------*/

typedef size_t VELOCT_type;

/*---------------------------------------------------------------------------
                            VELOC public functions
---------------------------------------------------------------------------*/

/**************************
 * Init / Finalize
 *************************/

int VELOC_Init(const char *cfg);
int VELOC_Finalize();
    
/**************************
 * Memory registration
 *************************/

// define new memory type for use in VELOC_Mem_protect
//   OUT type - defines a VELOC type for use in calls to Mem_protect (handle)
//   IN  size - size of type in bytes
void VELOC_Mem_type(VELOCT_type *t, size_t size);

// registers a memory region for checkpoint/restart
//   IN id    - application defined integer label for memory region
//   IN ptr   - pointer to start of memory region
//   IN count - number of consecutive elements in memory region
//   IN type  - type of element in memory region
int VELOC_Mem_protect(int id, void *ptr, size_t count, VELOCT_type type);

// unregisters a memory region
//   IN id    - application defined integer label for memory region
int VELOC_Mem_unprotect(int id);

/**************************
 * Checkpoint routines
 *************************/

// determine whether application should checkpoint
//   OUT flag - flag returns 1 if checkpoint should be taken, 0 otherwise
int VELOC_Checkpoint_test(int *flag);

// mark start of checkpoint phase
int VELOC_Checkpoint_begin(int id, const char *name);

// write registered memory regions into a checkpoint file
// must be called between VELOC_Checkpoint_begin/VELOC_Checkpoint_end
int VELOC_Checkpoint_mem(int id, int level);

// mark end of checkpoint phase
//   IN valid - calling process should set this flag to 1 if it wrote all checkpoint data successfully
int VELOC_Checkpoint_end(int id, int valid);

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _VELOC_H  ----- */
