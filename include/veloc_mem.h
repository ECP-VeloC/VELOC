/**
 *  @file   fti.h
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   July, 2013
 *  @brief  Header file for the VELOC_Mem library.
 */

#ifndef _VELOC_Mem_H
#define _VELOC_Mem_H

#include <mpi.h>

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Define RED color for VELOC_Mem output.                                       */
#define RED   "\x1B[31m"
/** Define ORANGE color for VELOC_Mem output.                                    */
#define ORG   "\x1B[38;5;202m"
/** Define GREEN color for VELOC_Mem output.                                     */
#define GRN   "\x1B[32m"
/** Define color RESET for VELOC_Mem output.                                     */
#define RESET "\x1B[0m"

/** Standard size of buffer and max node size.                             */
#define VELOC_Mem_BUFS 256
/** Word size used during RS encoding.                                     */
#define VELOC_Mem_WORD 16
/** Token returned when VELOC_Mem performs a checkpoint.                         */
#define VELOC_Mem_DONE 1
/** Token returned if a VELOC_Mem function succeeds.                             */
#define VELOC_Mem_SCES 0
/** Token returned if a VELOC_Mem function fails.                                */
#define VELOC_Mem_NSCS -1

/** Verbosity level to print only errors.                                  */
#define VELOC_Mem_EROR 4
/** Verbosity level to print only warning and errors.                      */
#define VELOC_Mem_WARN 3
/** Verbosity level to print main information.                             */
#define VELOC_Mem_INFO 2
/** Verbosity level to print debug messages.                               */
#define VELOC_Mem_DBUG 1

/** Token for checkpoint Baseline.                                         */
#define VELOC_Mem_BASE 990
/** Token for checkpoint Level 1.                                          */
#define VELOC_Mem_CKTW 991
/** Token for checkpoint Level 2.                                          */
#define VELOC_Mem_XORW 992
/** Token for checkpoint Level 3.                                          */
#define VELOC_Mem_RSEW 993
/** Token for checkpoint Level 4.                                          */
#define VELOC_Mem_PFSW 994
/** Token for end of the execution.                                        */
#define VELOC_Mem_ENDW 995
/** Token to reject checkpoint.                                            */
#define VELOC_Mem_REJW 996
/** Token for IO mode Posix.                                               */
#define VELOC_Mem_IO_POSIX 1001
/** Token for IO mode MPI.                                                 */
#define VELOC_Mem_IO_MPI 1002

/** Hashed string length.                                                */
#define MD5_DIGEST_LENGTH 17

#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
    /** Token for IO mode SIONlib.                                         */
    #define VELOC_Mem_IO_SIONLIB 1003
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
                                  New types
---------------------------------------------------------------------------*/

/** @typedef    VELOCT_double
 *  @brief      Double mapped as two integers to allow bit-wise operations.
 *
 *  Double mapped as integer and byte array to allow bit-wise operators so
 *  that we can inject failures on it.
 */
typedef union VELOCT_double {
    double          value;              /**< Double floating point value.   */
    float           floatval[2];        /**< Float mapped to do bit edits.  */
    int             intval[2];          /**< Integer mapped to do bit edits.*/
    char            byte[8];            /**< Byte array for coarser control.*/
} VELOCT_double;

/** @typedef    VELOCT_float
 *  @brief      Float mapped as integer to allow bit-wise operations.
 *
 *  Float mapped as integer and byte array to allow bit-wise operators so
 *  that we can inject failures on it.
 */
typedef union VELOCT_float {
    float           value;              /**< Floating point value.          */
    int             intval;             /**< Integer mapped to do bit edits.*/
    char            byte[4];            /**< Byte array for coarser control.*/
} VELOCT_float;

/** @typedef    VELOCT_type
 *  @brief      Type recognized by VELOC_Mem.
 *
 *  This type allows handling data structures.
 */
typedef struct VELOCT_type {
    int             id;                 /**< ID of the data type.           */
    int             size;               /**< Size of the data type.         */
} VELOCT_type;

/** @typedef    VELOCT_dataset
 *  @brief      Dataset metadata.
 *
 *  This type stores the metadata related with a dataset.
 */
typedef struct VELOCT_dataset {
    int             id;                 /**< ID to search/update dataset.   */
    void            *ptr;               /**< Pointer to the dataset.        */
    long            count;              /**< Number of elements in dataset. */
    VELOCT_type       type;               /**< Data type for the dataset.     */
    int             eleSize;            /**< Element size for the dataset.  */
    long            size;               /**< Total size of the dataset.     */
    /** MD5 Checksum                    */
    char            checksum[MD5_DIGEST_LENGTH];
} VELOCT_dataset;

/** @typedef    VELOCT_metadata
 *  @brief      Metadata for restart.
 *
 *  This type stores all the metadata necessary for the restart.
 */
typedef struct VELOCT_metadata {
    int*             exists;             /**< True if metadata exists        */
    long*            maxFs;              /**< Maximum file size.             */
    long*            fs;                 /**< File size.                     */
    long*            pfs;                /**< Partner file size.             */
    char*            ckptFile;           /**< Ckpt file name. [VELOC_Mem_BUFS]     */
} VELOCT_metadata;

/** @typedef    VELOCT_execution
 *  @brief      Execution metadata.
 *
 *  This type stores all the dynamic metadata related to the current execution
 */
typedef struct VELOCT_execution {
    char            id[VELOC_Mem_BUFS];       /**< Execution ID.                  */
    int             ckpt;               /**< Checkpoint flag.               */
    int             reco;               /**< Recovery flag.                 */
    int             ckptLvel;           /**< Checkpoint level.              */
    int             ckptIntv;           /**< Ckpt. interval in minutes.     */
    int             lastCkptLvel;       /**< Last checkpoint level.         */
    int             wasLastOffline;     /**< TRUE if last ckpt. offline.    */
    double          iterTime;           /**< Current wall time.             */
    double          lastIterTime;       /**< Time spent in the last iter.   */
    double          meanIterTime;       /**< Mean iteration time.           */
    double          globMeanIter;       /**< Global mean iteration time.    */
    double          totalIterTime;      /**< Total main loop time spent.    */
    unsigned int    syncIter;           /**< To check mean iter. time.      */
    unsigned int    minuteCnt;          /**< Checkpoint minute counter.     */
    unsigned int    ckptCnt;            /**< Checkpoint number counter.     */
    unsigned int    ckptIcnt;           /**< Iteration loop counter.        */
    unsigned int    ckptID;             /**< Checkpoint ID.                 */
    unsigned int    ckptNext;           /**< Iteration for next checkpoint. */
    unsigned int    ckptLast;           /**< Iteration for last checkpoint. */
    long            ckptSize;           /**< Checkpoint size.               */
    unsigned int    nbVar;              /**< Number of protected variables. */
    unsigned int    nbType;             /**< Number of data types.          */
    VELOCT_metadata   meta[5];            /**< Metadata for each ckpt level   */
    MPI_Comm        globalComm;         /**< Global communicator.           */
    MPI_Comm        groupComm;          /**< Group communicator.            */
} VELOCT_execution;

/** @typedef    VELOCT_configuration
 *  @brief      Configuration metadata.
 *
 *  This type stores the general configuration metadata.
 */
typedef struct VELOCT_configuration {
    char            cfgFile[VELOC_Mem_BUFS];  /**< Configuration file name.       */
    int             saveLastCkpt;       /**< TRUE to save last checkpoint.  */
    int             verbosity;          /**< Verbosity level.               */
    int             blockSize;          /**< Communication block size.      */
    int             transferSize;       /**< Transfer size local to PFS     */
    int             tag;                /**< Tag for MPI messages in VELOC_Mem.   */
    int             test;               /**< TRUE if local test.            */
    int             l3WordSize;         /**< RS encoding word size.         */
    int             ioMode;             /**< IO mode for L4 ckpt.           */
    char            localDir[VELOC_Mem_BUFS]; /**< Local directory.               */
    char            glbalDir[VELOC_Mem_BUFS]; /**< Global directory.              */
    char            metadDir[VELOC_Mem_BUFS]; /**< Metadata directory.            */
    char            lTmpDir[VELOC_Mem_BUFS];  /**< Local temporary directory.     */
    char            gTmpDir[VELOC_Mem_BUFS];  /**< Global temporary directory.    */
    char            mTmpDir[VELOC_Mem_BUFS];  /**< Metadata temporary directory.  */
} VELOCT_configuration;

/** @typedef    VELOCT_topology
 *  @brief      Topology metadata.
 *
 *  This type stores the topology metadata.
 */
typedef struct VELOCT_topology {
    int             nbProc;             /**< Total global number of proc.   */
    int             nbNodes;            /**< Total global number of nodes.  */
    int             myRank;             /**< My rank on the global comm.    */
    int             splitRank;          /**< My rank on the VELOC_Mem comm.       */
    int             nodeSize;           /**< Total number of pro. per node. */
    int             nbHeads;            /**< Number of VELOC_Mem proc. per node.  */
    int             nbApprocs;          /**< Number of app. proc. per node. */
    int             groupSize;          /**< Group size for L2 and L3.      */
    int             sectorID;           /**< Sector ID in the system.       */
    int             nodeID;             /**< Node ID in the system.         */
    int             groupID;            /**< Group ID in the node.          */
    int             amIaHead;           /**< TRUE if VELOC_Mem process.           */
    int             headRank;           /**< Rank of the head in this node. */
    int             nodeRank;           /**< Rank of the node.              */
    int             groupRank;          /**< My rank in the group comm.     */
    int             right;              /**< Proc. on the right of the ring.*/
    int             left;               /**< Proc. on the left of the ring. */
    int             body[VELOC_Mem_BUFS];     /**< List of app. proc. in the node.*/
} VELOCT_topology;


/** @typedef    VELOCT_checkpoint
 *  @brief      Checkpoint metadata.
 *
 *  This type stores all the checkpoint metadata.
 */
typedef struct VELOCT_checkpoint {
    char            dir[VELOC_Mem_BUFS];      /**< Checkpoint directory.          */
    char            metaDir[VELOC_Mem_BUFS];  /**< Metadata directory.            */
    int             isInline;           /**< TRUE if work is inline.        */
    int             ckptIntv;           /**< Checkpoint interval.           */
    int             ckptCnt;            /**< Checkpoint counter.            */

} VELOCT_checkpoint;

/** @typedef    VELOCT_injection
 *  @brief      Type to describe failure injections in VELOC_Mem.
 *
 *  This type allows users to describe a SDC failure injection model.
 */
typedef struct VELOCT_injection {
    int             rank;               /**< Rank of proc. that injects     */
    int             index;              /**< Array index of the bit-flip.   */
    int             position;           /**< Bit position of the bit-flip.  */
    int             number;             /**< Number of bit-flips to inject. */
    int             frequency;          /**< Injection frequency (in min.)  */
    int             counter;            /**< Injection counter.             */
    double          timer;              /**< Timer to measure frequency     */
} VELOCT_injection;

/*---------------------------------------------------------------------------
                                  Global variables
---------------------------------------------------------------------------*/

/** MPI communicator that splits the global one into app and VELOC_Mem appart.   */
extern MPI_Comm VELOC_Mem_COMM_WORLD;

/** VELOC_Mem data type for chars.                                               */
extern VELOCT_type VELOC_CHAR;
/** VELOC_Mem data type for short integers.                                      */
extern VELOCT_type VELOC_SHRT;
/** VELOC_Mem data type for integers.                                            */
extern VELOCT_type VELOC_INTG;
/** VELOC_Mem data type for long integers.                                       */
extern VELOCT_type VELOC_LONG;
/** VELOC_Mem data type for unsigned chars.                                      */
extern VELOCT_type VELOC_UCHR;
/** VELOC_Mem data type for unsigned short integers.                             */
extern VELOCT_type VELOC_USHT;
/** VELOC_Mem data type for unsigned integers.                                   */
extern VELOCT_type VELOC_UINT;
/** VELOC_Mem data type for unsigned long integers.                              */
extern VELOCT_type VELOC_ULNG;
/** VELOC_Mem data type for single floating point.                               */
extern VELOCT_type VELOC_SFLT;
/** VELOC_Mem data type for double floating point.                               */
extern VELOCT_type VELOC_DBLE;
/** VELOC_Mem data type for long doble floating point.                           */
extern VELOCT_type VELOC_LDBE;

/*---------------------------------------------------------------------------
                            VELOC_Mem public functions
---------------------------------------------------------------------------*/

int VELOC_Mem_Init(char *configFile, MPI_Comm globalComm);
int VELOC_Mem_Status();
int VELOC_Mem_InitType(VELOCT_type* type, int size);
int VELOC_Mem_Protect(int id, void* ptr, long count, VELOCT_type type);
int VELOC_Mem_BitFlip(int datasetID);
int VELOC_Mem_Checkpoint(int id, int level);
int VELOC_Mem_Recover();
int VELOC_Mem_Snapshot();
int VELOC_Mem_Finalize();

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _VELOC_Mem_H  ----- */
