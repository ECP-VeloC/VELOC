/** @brief Interface to call VELOC from Fortran
 *
 * @file   velocf.c
 * @author Florence Monna <fmonna@anl.gov>
 * @date 2018-02-15
 */

#include "include/veloc.h"
#include "velocf.h"

/**
 *  @brief      Fortran wrapper for VELOC_Init, Initializes VELOC.i
 *  @param      comm           The MPI communicator.
 *  @param      configFile     The name of the configuration file.
 *  @return     ierr           The error status of VELOC.
 *
 *  This function intializes VELOC with the parameters given in the 
 *  configuration file and the MPI communicator.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Init_fort_wrapper(int* comm, char* configFile)
{
    int ierr = VELOC_Init(MPI_Comm_f2c(*comm), configFile);
    return ierr;
}

/**
 *  @brief      Fortran wrapper for VELOC_Checkpoint_begin.
 *  @param      name           Name to give to the checkpoint.
 *  @param      version        The version of the checkpoint to create.
 *  @return     ierr           The error status of VELOC
 *
 *  This function starts the checkpointing process for the given checkpoint
 *  name and version.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Checkpoint_begin_wrapper(char *name, int version)
{
    int ierr = VELOC_Checkpoint_begin(name, version);
    return ierr;
}

/**
 *  @brief      Fortran wrapper for VELOC_Checkpoint.
 *  @param      name           Name to give to the checkpoint.
 *  @param      version        The version of the checkpoint to create.
 *  @return     ierr           The error status of VELOC
 *
 *  This function waits for the previous checkpoint phase to end (if in 
 *  asynchronous mode), starts a new checkpoint phase for the given checkpoint
 *  name and version, writes all registered memory regions and close the 
 *  checkpoint phase.
 *  Note: the corresponding C function is a convenience wrapper for 
 *  functions VELOC_Checkpoint_wait, VELOC_Checkpoint_begin, 
 *  VELOC_Checkpoint_mem, VELOC_Checkpoint_end.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Checkpoint_wrapper(char *name, int version)
{
    int ierr = VELOC_Checkpoint(name, version);
    return ierr;
}

/**
 *  @brief      Fortran wrapper for VELOC_Mem_protect.
 *  @param      id              ID for searches and update.
 *  @param      ptr             Pointer to the data structure.
 *  @param      count           Number of elements in the data structure.
 *  @param      baze_size       Size of elements in the data structure.
 *  @return     integer         VELOC_SCES if successful.
 *
 *  This function stores a pointer to a data structure, its size, its ID,
 *  its number of elements and the type of the elements. This list of
 *  structures is the data that will be stored during a checkpoint and
 *  loaded during a recovery.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_protect_wrapper(int id, void* ptr, long count, long base_size)
{
    return VELOC_Mem_protect(id, ptr, count, base_size);
}

/**
 *  @brief      Fortran wrapper for VELOC_Restart_test.
 *  @param	name             Name of the checkpoint to look for.
 *  @param      needed_version   The version we are looking to get close to.
 *  @return     version          The closest version to needed_version.
 *
 *  This function looks for checkpoints with a given name, and tries to get 
 *  as close as possible to needed_version. 
 *  If it finds a checkpoint close enough, it returns the version of the 
 *  checkpoint. 
 *  If it does not find a checkpoint or not one close enough, it returns -1.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Restart_test_wrapper(char *name, int needed_version)
{
    int version = VELOC_Restart_test(name, needed_version);
    return version;
}
   
/**
 *  @brief      Fortran wrapper for VELOC_Restart_begin.
 *  @param	name          Name of the checkpoint to look for.
 *  @param      version       The version we are looking for.
 *  @return     ierr          VELOC_SCES if successful, VELOC_NSCS if not.
 *  
 *  This function looks for the checkpoint with a given name and version. 
 *  When found it starts the restart process.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Restart_begin_wrapper(char *name, int version)
{
    int ierr = VELOC_Restart_begin(name, version);
    return ierr;
}

/**
 *  @brief      Fortran wrapper for VELOC_Restart.
 *  @param	name          Name of the checkpoint to look for.
 *  @param      version       The version we are looking for.
 *  @return     ierr          VELOC_SCES if successful, VELOC_NSCS if not.
 *  
 *  This function looks for the checkpoint with a given name and version. 
 *  When found it starts the restart process, recover the data and ends the 
 *  restart process. 
 *  Note: the corresponding C function is a convenience wrapper for 
 *  functions VELOC_Restart_begin, VELOC_Recover_mem, VELOC_Restart_end.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Restart_wrapper(char *name, int version)
{
    int ierr = VELOC_Restart(name, version);
    return ierr;
}

/**
 *  @brief      Fortran wrapper for VELOC_Route_file.
 *  @param	ckpt_file_name   Name of the user's checkpoint file.
 *  @return     ierr             VELOC_SCES if successful, VELOC_NSCS if not.
 *  
 *  This function looks for the manual checkpoint file and performs the I/O.
 *
 **/
/*-------------------------------------------------------------------------*/
int VELOC_Route_file_wrapper(char *ckpt_file_name, char *routed)
{
    int ierr = VELOC_Route_file(ckpt_file_name, routed);
    return ierr;
}
