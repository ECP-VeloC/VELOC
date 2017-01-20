#ifndef _VELOC_H
#define _VELOC_H

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Token returned if a VEOC function succeeds.                             */
#define VELOC_SUCCESS (0)
#define VELOC_FAILURE (1)

#define VELOC_MAX_NAME (1024)

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
                                  New types
---------------------------------------------------------------------------*/

/** @typedef    VELOCT_type
 *  @brief      Type recognized by VELOC.
 *
 *  This type allows handling data structures.
 */
typedef struct VELOCT_type {            /** VELOC type declarator.         */
    int             id;                 /** ID of the data type.           */
    int             size;               /** Size of the data type.         */
} VELOCT_type;

/*---------------------------------------------------------------------------
                                  Global variables
---------------------------------------------------------------------------*/

/** VELOC data type for chars.                                               */
extern VELOCT_type VELOC_CHAR;
/** VELOC data type for short integers.                                      */
extern VELOCT_type VELOC_SHRT;
/** VELOC data type for integers.                                            */
extern VELOCT_type VELOC_INTG;
/** VELOC data type for long integers.                                       */
extern VELOCT_type VELOC_LONG;
/** VELOC data type for unsigned chars.                                      */
extern VELOCT_type VELOC_UCHR;
/** VELOC data type for unsigned short integers.                             */
extern VELOCT_type VELOC_USHT;
/** VELOC data type for unsigned integers.                                   */
extern VELOCT_type VELOC_UINT;
/** VELOC data type for unsigned long integers.                              */
extern VELOCT_type VELOC_ULNG;
/** VELOC data type for single floating point.                               */
extern VELOCT_type VELOC_SFLT;
/** VELOC data type for double floating point.                               */
extern VELOCT_type VELOC_DBLE;
/** VELOC data type for long doble floating point.                           */
extern VELOCT_type VELOC_LDBE;

/*---------------------------------------------------------------------------
                            VELOC public functions
---------------------------------------------------------------------------*/

/**************************
 * Init / Finalize
 *************************/

// can set config == NULL
int VELOC_Init(char *config);

int VELOC_Finalize();

/**************************
 * Memory registration
 *************************/

// define new memory type
int VELOC_Mem_type(VELOCT_type* type, int size);

// mark memory for checkpoint
int VELOC_Mem_protect(int id, void* ptr, long count, VELOCT_type type);

/**************************
 * File registration
 *************************/

// like SCR_Route_file
int VELOC_Route_file(const char* name, char* veloc_name);

/**************************
 * Restart routines
 *************************/

// flag returns 1 if there is a checkpoint available to read, 0 otherwise
int VELOC_Have_restart(int* flag);

int VELOC_Start_restart();

// reads protected memory from file
int VELOC_Mem_restart();

int VELOC_Complete_restart();

/**************************
 * Checkpoint routines
 *************************/

// flag returns 1 if checkpoint should be taken, 0 otherwise
int VELOC_Need_checkpoint(int* flag);

int VELOC_Start_checkpoint();

// writes protected memory to file
int VELOC_Mem_checkpoint();

int VELOC_Complete_checkpoint(int valid);

/**************************
 * convenience functions for existing FTI users
 * (can be implemented fully with above functions)
 ************************/

// FTI_Checkpoint (without id and level params)
int VELOC_Mem_save();

// FTI_Recovery
int VELOC_Mem_recover();

// FTI_Snapshot
int VELOC_Mem_snapshot();

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _VELOC_H  ----- */
