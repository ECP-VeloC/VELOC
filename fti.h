/**
 *  @file   fti.h
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   July, 2013
 *  @brief  Header file for the FTI library.
 */

#ifndef _FTI_H
#define _FTI_H

#include <mpi.h>

/*---------------------------------------------------------------------------
                                  Defines
---------------------------------------------------------------------------*/

/** Standard size of buffer and mas node size.                             */
#define FTI_BUFS 256
/** Word size used during RS encoding.                                     */
#define FTI_WORD 16
/** Token returned when FTI performs a checkpoint.                         */
#define FTI_DONE 1
/** Token returned if a FTI function succeeds.                             */
#define FTI_SCES 0
/** Token returned if a FTI function fails.                                */
#define FTI_NSCS -1

/** Verbosity level to print only errors.                                  */
#define FTI_EROR 4
/** Verbosity level to print only warning and errors.                      */
#define FTI_WARN 3
/** Verbosity level to print main information.                             */
#define FTI_INFO 2
/** Verbosity level to print debug messages.                               */
#define FTI_DBUG 1

/** Token for checkpoint Baseline.                                         */
#define FTI_BASE 990
/** Token for checkpoint Level 1.                                          */
#define FTI_CKTW 991
/** Token for checkpoint Level 2.                                          */
#define FTI_XORW 992
/** Token for checkpoint Level 3.                                          */
#define FTI_RSEW 993
/** Token for checkpoint Level 4.                                          */
#define FTI_PFSW 994
/** Token for end of the execution.                                        */
#define FTI_ENDW 995
/** Token to reject checkpoint.                                            */
#define FTI_REJW 996

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
                                  New types
---------------------------------------------------------------------------*/

/** @typedef    FTIT_double
 *  @brief      Double mapped as two integers to allow bit-wise operations.
 *
 *  Double mapped as integer and byte array to allow bit-wise operators so
 *  that we can inject failures on it.
 */
typedef union FTIT_double {             /** Double mapped as a byte array. */
    double          value;              /** Double floating point value.   */
    float           floatval[2];        /** Float mapped to do bit edits.  */
    int             intval[2];          /** Integer mapped to do bit edits.*/
    char            byte[8];            /** Byte array for coarser control.*/
} FTIT_double;

/** @typedef    FTIT_float
 *  @brief      Float mapped as integer to allow bit-wise operations.
 *
 *  Float mapped as integer and byte array to allow bit-wise operators so
 *  that we can inject failures on it.
 */
typedef union FTIT_float {              /** Float mapped as a byte array.  */
    float           value;              /** Floating point value.          */
    int             intval;             /** Integer mapped to do bit edits.*/
    char            byte[4];            /** Byte array for coarser control.*/
} FTIT_float;

/** @typedef    FTIT_type
 *  @brief      Type recognized by FTI.
 *
 *  This type allows handling data structures.
 */
typedef struct FTIT_type {              /** FTI type declarator.           */
    int             id;                 /** ID of the data type.           */
    int             size;               /** Size of the data type.         */
} FTIT_type;

/** @typedef    FTIT_dataset
 *  @brief      Dataset metadata.
 *
 *  This type stores the metadata related with a dataset.
 */
typedef struct FTIT_dataset {           /** Dataset metadata.              */
    int             id;                 /** ID to search/update dataset.   */
    void            *ptr;               /** Pointer to the dataset.        */
    int             count;              /** Number of elements in dataset. */
    FTIT_type       type;               /** Data type for the dataset.     */
    int             eleSize;            /** Element size for the dataset.  */
    long            size;               /** Total size of the dataset.     */
} FTIT_dataset;

/** @typedef    FTIT_execution
 *  @brief      Execution metadata
 *
 *  This type stores all the dynamic metadata related to the current execution
 */
typedef struct FTIT_execution {         /** Execution metadata.            */
    char            id[FTI_BUFS];       /** Execution ID.                  */
    char            ckptFile[FTI_BUFS]; /** Checkpoint file name.          */
    int             ckpt;               /** Checkpoint flag.               */
    int             reco;               /** Recovery flag.                 */
    int             ckptLvel;           /** Checkpoint level.              */
    int             ckptIntv;           /** Ckpt. interval in minutes.     */
    int             lastCkptLvel;       /** Last checkpoint level.         */
    int             wasLastOffline;     /** TRUE if last ckpt. offline.    */
    double          iterTime;           /** Current wall time.             */
    double          lastIterTime;       /** Time spent in the last iter.   */
    double          meanIterTime;       /** Mean iteration time.           */
    double          globMeanIter;       /** Global mean iteration time.    */
    double          totalIterTime;      /** Total main loop time spent.    */
    unsigned int    syncIter;           /** To check mean iter. time.      */
    unsigned int    minuteCnt;          /** Checkpoint minute counter.     */
    unsigned int    ckptCnt;             /** Checkpoint number counter.     */
    unsigned int    ckptIcnt;           /** Iteration loop counter.        */
    unsigned int    ckptID;             /** Checkpoint ID.                 */
    unsigned int    ckptNext;           /** Iteration for next checkpoint. */
    unsigned int    ckptLast;           /** Iteration for last checkpoint. */
    unsigned int    ckptSize;           /** Checkpoint size.               */
    unsigned int    nbVar;              /** Number of protected variables. */
    unsigned int    nbType;             /** Number of data types.          */
    MPI_Comm        globalComm;         /** Global communicator.           */
    MPI_Comm        groupComm;          /** Group communicator.            */
} FTIT_execution;

/** @typedef    FTIT_configuration
 *  @brief      Configuration metadata.
 *
 *  This type stores the general configuration metadata.
 */
typedef struct FTIT_configuration {     /** Configuration metadata.        */
    char            cfgFile[FTI_BUFS];  /** Configuration file name.       */
    int             saveLastCkpt;       /** TRUE to save last checkpoint.  */
    int             verbosity;          /** Verbosity level.               */
    int             blockSize;          /** Communication block size.      */
    int             tag;                /** Tag for MPI messages in FTI.   */
    int             test;               /** TRUE if local test.            */
    int             l3WordSize;         /** RS encoding word size.         */
    char            localDir[FTI_BUFS]; /** Local directory.               */
    char            glbalDir[FTI_BUFS]; /** Global directory.              */
    char            metadDir[FTI_BUFS]; /** Metadata directory.            */
    char            lTmpDir[FTI_BUFS];  /** Local temporary directory.     */
    char            gTmpDir[FTI_BUFS];  /** Global temporary directory.    */
    char            mTmpDir[FTI_BUFS];  /** Metadata temporary directory.  */
} FTIT_configuration;

/** @typedef    FTIT_topology
 *  @brief      Topology metadata.
 *
 *  This type stores the topology metadata.
 */
typedef struct FTIT_topology {          /** Topology metadata.             */
    int             nbProc;             /** Total global number of proc.   */
    int             nbNodes;            /** Total global number of nodes.  */
    int             myRank;             /** My rank on the global comm.    */
    int             splitRank;          /** My rank on the FTI comm.       */
    int             nodeSize;           /** Total number of pro. per node. */
    int             nbHeads;            /** Number of FTI proc. per node.  */
    int             nbApprocs;          /** Number of app. proc. per node. */
    int             groupSize;          /** Group size for L2 and L3.      */
    int             sectorID;           /** Sector ID in the system.       */
    int             nodeID;             /** Node ID in the system.         */
    int             groupID;            /** Group ID in the node.          */
    int             amIaHead;           /** TRUE if FTI process.           */
    int             headRank;           /** Rank of the head in this node. */
    int             nodeRank;           /** Rank of the node.              */
    int             groupRank;          /** My rank in the group comm      */
    int             right;              /** Proc. on the right of the ring.*/
    int             left;               /** Proc. on the left of the ring. */
    int             body[FTI_BUFS];     /** List of app. proc. in the node.*/
} FTIT_topology;

/** @typedef    FTIT_checkpoint
 *  @brief      Checkpoint metadata.
 *
 *  This type stores all the checkpoint metadata.
 */
typedef struct FTIT_checkpoint {        /** Checkpoint metadata.           */
    char            dir[FTI_BUFS];      /** Checkpoint directory.          */
    char            metaDir[FTI_BUFS];  /** Metadata directory.            */
    int             isInline;           /** TRUE if work is inline.        */
    int             ckptIntv;           /** Checkpoint interval.           */
    int             ckptCnt;            /** Checkpoint counter.            */
} FTIT_checkpoint;

/** @typedef    FTIT_injection
 *  @brief      Type to describe failure injections in FTI.
 *
 *  This type allows users to describe a SDC failure injection model.
 */
typedef struct FTIT_injection {         /** FTI type declarator.           */
    int             rank;               /** Rank of proc. that injects     */
    int             index;              /** Array index of the bit-flip.   */
    int             position;           /** Bit position of the bit-flip.  */
    int             number;             /** Number of bit-flips to inject. */
    int             frequency;          /** Injection frequency (in min.)  */
    int             counter;            /** Injection counter.             */
    double          timer;              /** Timer to measure frequency     */
} FTIT_injection;

/*---------------------------------------------------------------------------
                                  Global variables
---------------------------------------------------------------------------*/

/** MPI communicator that splits the global one into app and FTI appart.   */
extern MPI_Comm FTI_COMM_WORLD;

/** FTI data type for chars.                                               */
extern FTIT_type FTI_CHAR;
/** FTI data type for short integers.                                      */
extern FTIT_type FTI_SHRT;
/** FTI data type for integers.                                            */
extern FTIT_type FTI_INTG;
/** FTI data type for long integers.                                       */
extern FTIT_type FTI_LONG;
/** FTI data type for unsigned chars.                                      */
extern FTIT_type FTI_UCHR;
/** FTI data type for unsigned short integers.                             */
extern FTIT_type FTI_USHT;
/** FTI data type for unsigned integers.                                   */
extern FTIT_type FTI_UINT;
/** FTI data type for unsigned long integers.                              */
extern FTIT_type FTI_ULNG;
/** FTI data type for single floating point.                               */
extern FTIT_type FTI_SFLT;
/** FTI data type for double floating point.                               */
extern FTIT_type FTI_DBLE;
/** FTI data type for long doble floating point.                           */
extern FTIT_type FTI_LDBE;

/*---------------------------------------------------------------------------
                            FTI public functions
---------------------------------------------------------------------------*/

int FTI_Init(char *configFile, MPI_Comm globalComm);
int FTI_Status();
int FTI_InitType(FTIT_type* type, int size);
int FTI_Protect(int id, void* ptr, long count, FTIT_type type);
int FTI_BitFlip(int datasetID);
int FTI_Checkpoint(int id, int level);
int FTI_Recover();
int FTI_Snapshot();
int FTI_Finalize();

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _FTI_H  ----- */
