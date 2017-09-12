/** @brief Interface to call VELOC_Mem from Fortran
 *
 * @file   velec_memf.c
 * @author Faysal Boui <faysal.boui@cea.fr>
 * @author Julien Bigot <julien.bigot@cea.fr>
 * @date 2013-08-01
 */

#include "velec_mem.h"
#include "interface.h"
#include "velec_memf.h"

/** @brief Fortran wrapper for VELOC_Mem_Init, Initializes VELOC_Mem.
 * 
 * @return the error status of VELOC_Mem
 * @param configFile (IN) the name of the configuration file as a
 *        \0 terminated string
 * @param globalComm (INOUT) the "world" communicator, VELOC_Mem will replace it
 *        with a communicator where its own processes have been removed.
 */
int VELOC_Mem_Init_fort_wrapper(char* configFile, int* globalComm)
{
    int ierr = VELOC_Mem_Init(configFile, MPI_Comm_f2c(*globalComm));
    *globalComm = MPI_Comm_c2f(VELOC_Mem_COMM_WORLD);
    return ierr;
}

/**
 *   @brief      Initializes a data type.
 *   @param      type            The data type to be intialized.
 *   @param      size            The size of the data type to be intialized.
 *   @return     integer         VELOC_SUCCESS if successful.
 *
 *   This function initalizes a data type. the only information needed is the
 *   size of the data type, the rest is black box for VELOC_Mem.
 *
 **/
int VELOC_Mem_type_wrapper(VELOCT_type** type, int size)
{
    *type = talloc(VELOCT_type, 1);
    return VELOC_Mem_type(*type, size);
}

/**
 @brief      Stores or updates a pointer to a variable that needs to be protected.
 @param      id              ID for searches and update.
 @param      ptr             Pointer to the data structure.
 @param      count           Number of elements in the data structure.
 @param      type            Type of elements in the data structure.
 @return     integer         VELOC_SUCCESS if successful.

 This function stores a pointer to a data structure, its size, its ID,
 its number of elements and the type of the elements. This list of
 structures is the data that will be stored during a checkpoint and
 loaded during a recovery.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Protect_wrapper(int id, void* ptr, long count, VELOCT_type* type)
{
    return VELOC_Mem_Protect(id, ptr, count, *type);
}
