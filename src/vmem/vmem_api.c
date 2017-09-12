/**
 *  @file   api.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   December, 2013
 *  @brief  API functions for the VELOC_Mem library.
 */

#include "interface.h"

/** General configuration information used by VELOC_Mem.                         */
static VELOCT_configuration VELOC_Mem_Conf;

/** Checkpoint information for all levels of checkpoint.                   */
static VELOCT_checkpoint VELOC_Mem_Ckpt[5];

/** Dynamic information for this execution.                                */
static VELOCT_execution VELOC_Mem_Exec;

/** Topology of the system.                                                */
static VELOCT_topology VELOC_Mem_Topo;

/** Array of datasets and all their internal information.                  */
static VELOCT_dataset VELOC_Mem_Data[VELOC_Mem_BUFS];

/** SDC injection model and all the required information.                  */
static VELOCT_injection VELOC_Mem_Inje;


/** MPI communicator that splits the global one into app and VELOC_Mem appart.   */
MPI_Comm VELOC_Mem_COMM_WORLD;

/** VELOC_Mem data type for chars.                                               */
VELOCT_type VELOC_CHAR;
/** VELOC_Mem data type for short integers.                                      */
VELOCT_type VELOC_SHRT;
/** VELOC_Mem data type for integers.                                            */
VELOCT_type VELOC_INTG;
/** VELOC_Mem data type for long integers.                                       */
VELOCT_type VELOC_LONG;
/** VELOC_Mem data type for unsigned chars.                                      */
VELOCT_type VELOC_UCHR;
/** VELOC_Mem data type for unsigned short integers.                             */
VELOCT_type VELOC_USHT;
/** VELOC_Mem data type for unsigned integers.                                   */
VELOCT_type VELOC_UINT;
/** VELOC_Mem data type for unsigned long integers.                              */
VELOCT_type VELOC_ULNG;
/** VELOC_Mem data type for single floating point.                               */
VELOCT_type VELOC_SFLT;
/** VELOC_Mem data type for double floating point.                               */
VELOCT_type VELOC_DBLE;
/** VELOC_Mem data type for long doble floating point.                           */
VELOCT_type VELOC_LDBE;


/*-------------------------------------------------------------------------*/
/**
    @brief      It aborts the application.

    This function aborts the application after cleaning the file system.

 **/
/*-------------------------------------------------------------------------*/
void VELOC_Mem_Abort()
{
    VELOC_Mem_Clean(&VELOC_Mem_Conf, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, 5, 0, VELOC_Mem_Topo.myRank);
    MPI_Abort(MPI_COMM_WORLD, -1);
    MPI_Finalize();
    exit(1);
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Initializes VELOC_Mem.
    @param      configFile      VELOC_Mem configuration file.
    @param      globalComm      Main MPI communicator of the application.
    @return     integer         VELOC_Mem_SCES if successful.

    This function initializes the VELOC_Mem context and prepares the heads to wait
    for checkpoints. VELOC_Mem processes should never get out of this function. In
    case of a restart, checkpoint files should be recovered and in place at the
    end of this function.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Init(char* configFile, MPI_Comm globalComm)
{
    VELOC_Mem_Exec.globalComm = globalComm;
    MPI_Comm_rank(VELOC_Mem_Exec.globalComm, &VELOC_Mem_Topo.myRank);
    MPI_Comm_size(VELOC_Mem_Exec.globalComm, &VELOC_Mem_Topo.nbProc);
    snprintf(VELOC_Mem_Conf.cfgFile, VELOC_Mem_BUFS, "%s", configFile);
    VELOC_Mem_Conf.verbosity = 1;
    VELOC_Mem_Inje.timer = MPI_Wtime();
    VELOC_Mem_COMM_WORLD = globalComm; // Temporary before building topology
    VELOC_Mem_Topo.splitRank = VELOC_Mem_Topo.myRank; // Temporary before building topology
    int res = VELOC_Mem_Try(VELOC_Mem_LoadConf(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, &VELOC_Mem_Inje), "load configuration.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Abort();
    }
    res = VELOC_Mem_Try(VELOC_Mem_Topology(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo), "build topology.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Abort();
    }
    VELOC_Mem_Try(VELOC_Mem_InitBasicTypes(VELOC_Mem_Data), "create the basic data types.");
    if (VELOC_Mem_Topo.myRank == 0) {
        VELOC_Mem_Try(VELOC_Mem_UpdateConf(&VELOC_Mem_Conf, &VELOC_Mem_Exec, VELOC_Mem_Exec.reco), "update configuration file.");
    }
    MPI_Barrier(VELOC_Mem_Exec.globalComm); //wait for myRank == 0 process to save config file
    VELOC_Mem_MallocMeta(&VELOC_Mem_Exec, &VELOC_Mem_Topo);
    res = VELOC_Mem_Try(VELOC_Mem_LoadMeta(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt), "load metadata");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Abort();
    }
    if (VELOC_Mem_Topo.amIaHead) { // If I am a VELOC_Mem dedicated process
        if (VELOC_Mem_Exec.reco) {
            res = VELOC_Mem_Try(VELOC_Mem_RecoverFiles(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt), "recover the checkpoint files.");
            if (res != VELOC_Mem_SCES) {
                VELOC_Mem_Abort();
            }
        }
        res = 0;
        while (res != VELOC_Mem_ENDW) {
            res = VELOC_Mem_Listen(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt);
        }
        VELOC_Mem_Print("Head stopped listening.", VELOC_Mem_DBUG);
        VELOC_Mem_Finalize();
    }
    else { // If I am an application process
        if (VELOC_Mem_Exec.reco) {
            res = VELOC_Mem_Try(VELOC_Mem_RecoverFiles(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt), "recover the checkpoint files.");
            if (res != VELOC_Mem_SCES) {
                VELOC_Mem_Abort();
            }
            VELOC_Mem_Exec.ckptCnt = VELOC_Mem_Exec.ckptID;
            VELOC_Mem_Exec.ckptCnt++;
        }
    }
    VELOC_Mem_Print("VELOC_Mem has been initialized.", VELOC_Mem_INFO);
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It returns the current status of the recovery flag.
    @return     integer         VELOC_Mem_Exec.reco.

    This function returns the current status of the recovery flag.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Status()
{
    return VELOC_Mem_Exec.reco;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It initializes a data type.
    @param      type            The data type to be intialized.
    @param      size            The size of the data type to be intialized.
    @return     integer         VELOC_Mem_SCES if successful.

    This function initalizes a data type. the only information needed is the
    size of the data type, the rest is black box for VELOC_Mem.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_InitType(VELOCT_type* type, int size)
{
    type->id = VELOC_Mem_Exec.nbType;
    type->size = size;
    VELOC_Mem_Exec.nbType = VELOC_Mem_Exec.nbType + 1;
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It sets/resets the pointer and type to a protected variable.
    @param      id              ID for searches and update.
    @param      ptr             Pointer to the data structure.
    @param      count           Number of elements in the data structure.
    @param      type            Type of elements in the data structure.
    @return     integer         VELOC_Mem_SCES if successful.

    This function stores a pointer to a data structure, its size, its ID,
    its number of elements and the type of the elements. This list of
    structures is the data that will be stored during a checkpoint and
    loaded during a recovery. It resets the pointer to a data structure,
    its size, its number of elements and the type of the elements if the
    dataset was already previously registered.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Protect(int id, void* ptr, long count, VELOCT_type type)
{
    int i, prevSize, updated = 0;
    char str[VELOC_Mem_BUFS];
    float ckptSize;
    for (i = 0; i < VELOC_Mem_BUFS; i++) {
        if (id == VELOC_Mem_Data[i].id) {
            prevSize = VELOC_Mem_Data[i].size;
            VELOC_Mem_Data[i].ptr = ptr;
            VELOC_Mem_Data[i].count = count;
            VELOC_Mem_Data[i].type = type;
            VELOC_Mem_Data[i].eleSize = type.size;
            VELOC_Mem_Data[i].size = type.size * count;
            VELOC_Mem_Exec.ckptSize = VELOC_Mem_Exec.ckptSize + (type.size * count) - prevSize;
            updated = 1;
        }
    }
    if (updated) {
        ckptSize = VELOC_Mem_Exec.ckptSize / (1024.0 * 1024.0);
        sprintf(str, "Variable ID %d reseted. Current ckpt. size per rank is %.2fMB.", id, ckptSize);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    }
    else {
        if (VELOC_Mem_Exec.nbVar >= VELOC_Mem_BUFS) {
            VELOC_Mem_Print("Too many variables registered.", VELOC_Mem_EROR);
            VELOC_Mem_Clean(&VELOC_Mem_Conf, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, 5, VELOC_Mem_Topo.groupID, VELOC_Mem_Topo.myRank);
            MPI_Abort(MPI_COMM_WORLD, -1);
            MPI_Finalize();
            exit(1);
        }
        VELOC_Mem_Data[VELOC_Mem_Exec.nbVar].id = id;
        VELOC_Mem_Data[VELOC_Mem_Exec.nbVar].ptr = ptr;
        VELOC_Mem_Data[VELOC_Mem_Exec.nbVar].count = count;
        VELOC_Mem_Data[VELOC_Mem_Exec.nbVar].type = type;
        VELOC_Mem_Data[VELOC_Mem_Exec.nbVar].eleSize = type.size;
        VELOC_Mem_Data[VELOC_Mem_Exec.nbVar].size = type.size * count;
        VELOC_Mem_Exec.nbVar = VELOC_Mem_Exec.nbVar + 1;
        VELOC_Mem_Exec.ckptSize = VELOC_Mem_Exec.ckptSize + (type.size * count);
        ckptSize = VELOC_Mem_Exec.ckptSize / (1024.0 * 1024.0);
        sprintf(str, "Variable ID %d to protect. Current ckpt. size per rank is %.2fMB.", id, ckptSize);
        VELOC_Mem_Print(str, VELOC_Mem_INFO);
    }
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It corrupts a bit of the given float.
    @param      target          Pointer to the float to corrupt.
    @param      bit             Position of the bit to corrupt.
    @return     integer         VELOC_Mem_SCES if successful.

    This function filps the bit of the target float.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_FloatBitFlip(float* target, int bit)
{
    if (bit >= 32 || bit < 0) {
        return VELOC_Mem_NSCS;
    }
    int* corIntPtr = (int*)target;
    int corInt = *corIntPtr;
    corInt = corInt ^ (1 << bit);
    corIntPtr = &corInt;
    float* fp = (float*)corIntPtr;
    *target = *fp;
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It corrupts a bit of the given float.
    @param      target          Pointer to the float to corrupt.
    @param      bit             Position of the bit to corrupt.
    @return     integer         VELOC_Mem_SCES if successful.

    This function filps the bit of the target float.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_DoubleBitFlip(double* target, int bit)
{
    if (bit >= 64 || bit < 0) {
        return VELOC_Mem_NSCS;
    }
    VELOCT_double myDouble;
    myDouble.value = *target;
    int bitf = (bit >= 32) ? bit - 32 : bit;
    int half = (bit >= 32) ? 1 : 0;
    VELOC_Mem_FloatBitFlip(&(myDouble.floatval[half]), bitf);
    *target = myDouble.value;
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Bit-flip injection following the injection instructions.
    @param      datasetID       ID of the dataset where to inject.
    @return     integer         VELOC_Mem_SCES if successful.

    This function injects the given number of bit-flips, at the given
    frequency and in the given location (rank, dataset, bit position).

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_BitFlip(int datasetID)
{
    if (VELOC_Mem_Inje.rank == VELOC_Mem_Topo.splitRank) {
        if (datasetID >= VELOC_Mem_Exec.nbVar) {
            return VELOC_Mem_NSCS;
        }
        if (VELOC_Mem_Inje.counter < VELOC_Mem_Inje.number) {
            if ((MPI_Wtime() - VELOC_Mem_Inje.timer) > VELOC_Mem_Inje.frequency) {
                if (VELOC_Mem_Inje.index < VELOC_Mem_Data[datasetID].count) {
                    char str[VELOC_Mem_BUFS];
                    if (VELOC_Mem_Data[datasetID].type.id == 9) { // If it is a double
                        double* target = VELOC_Mem_Data[datasetID].ptr + VELOC_Mem_Inje.index;
                        double ori = *target;
                        int res = VELOC_Mem_DoubleBitFlip(target, VELOC_Mem_Inje.position);
                        VELOC_Mem_Inje.counter = (res == VELOC_Mem_SCES) ? VELOC_Mem_Inje.counter + 1 : VELOC_Mem_Inje.counter;
                        VELOC_Mem_Inje.timer = (res == VELOC_Mem_SCES) ? MPI_Wtime() : VELOC_Mem_Inje.timer;
                        sprintf(str, "Injecting bit-flip in dataset %d, index %d, bit %d : %f => %f",
                            datasetID, VELOC_Mem_Inje.index, VELOC_Mem_Inje.position, ori, *target);
                        VELOC_Mem_Print(str, VELOC_Mem_WARN);
                        return res;
                    }
                    if (VELOC_Mem_Data[datasetID].type.id == 8) { // If it is a float
                        float* target = VELOC_Mem_Data[datasetID].ptr + VELOC_Mem_Inje.index;
                        float ori = *target;
                        int res = VELOC_Mem_FloatBitFlip(target, VELOC_Mem_Inje.position);
                        VELOC_Mem_Inje.counter = (res == VELOC_Mem_SCES) ? VELOC_Mem_Inje.counter + 1 : VELOC_Mem_Inje.counter;
                        VELOC_Mem_Inje.timer = (res == VELOC_Mem_SCES) ? MPI_Wtime() : VELOC_Mem_Inje.timer;
                        sprintf(str, "Injecting bit-flip in dataset %d, index %d, bit %d : %f => %f",
                            datasetID, VELOC_Mem_Inje.index, VELOC_Mem_Inje.position, ori, *target);
                        VELOC_Mem_Print(str, VELOC_Mem_WARN);
                        return res;
                    }
                }
            }
        }
    }
    return VELOC_Mem_NSCS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It takes the checkpoint and triggers the post-ckpt. work.
    @param      id              Checkpoint ID.
    @param      level           Checkpoint level.
    @return     integer         VELOC_Mem_SCES if successful.

    This function starts by blocking on a receive if the previous ckpt. was
    offline. Then, it updates the ckpt. information. It writes down the ckpt.
    data, creates the metadata and the post-processing work. This function
    is complementary with the VELOC_Mem_Listen function in terms of communications.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Checkpoint(int id, int level)
{
    int res = VELOC_Mem_NSCS, value;
    double t0, t1, t2, t3;
    char str[VELOC_Mem_BUFS];
    char catstr[VELOC_Mem_BUFS];
    int ckptFirst = !VELOC_Mem_Exec.ckptID;
    VELOC_Mem_Exec.ckptID = id;
    MPI_Status status;
    if ((level > 0) && (level < 5)) {
        t0 = MPI_Wtime();
        VELOC_Mem_Exec.ckptLvel = level; // (1) TODO #BUG? this should come after (2)
        // str is set to print ckpt information on stdout
        sprintf(catstr, "Ckpt. ID %d", VELOC_Mem_Exec.ckptID);
        sprintf(str, "%s (L%d) (%.2f MB/proc)", catstr, VELOC_Mem_Exec.ckptLvel, VELOC_Mem_Exec.ckptSize / (1024.0 * 1024.0));
        if (VELOC_Mem_Exec.wasLastOffline == 1) { // Block until previous checkpoint is done (Async. work)
            MPI_Recv(&res, 1, MPI_INT, VELOC_Mem_Topo.headRank, VELOC_Mem_Conf.tag, VELOC_Mem_Exec.globalComm, &status);
            if (res == VELOC_Mem_SCES) {
                VELOC_Mem_Exec.lastCkptLvel = res; // TODO why this assignment ??
                VELOC_Mem_Exec.wasLastOffline = 1;
                VELOC_Mem_Exec.lastCkptLvel = VELOC_Mem_Exec.ckptLvel; // (2) TODO look at (1)
            }
        }
        t1 = MPI_Wtime();
        res = VELOC_Mem_Try(VELOC_Mem_WriteCkpt(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, VELOC_Mem_Data), "write the checkpoint.");
        t2 = MPI_Wtime();
        if (!VELOC_Mem_Ckpt[VELOC_Mem_Exec.ckptLvel].isInline) { // If postCkpt. work is Async. then send message..
            VELOC_Mem_Exec.wasLastOffline = 1;
            if (res != VELOC_Mem_SCES) {
                value = VELOC_Mem_REJW;
            }
            else {
                value = VELOC_Mem_BASE + VELOC_Mem_Exec.ckptLvel;
            }
            MPI_Send(&value, 1, MPI_INT, VELOC_Mem_Topo.headRank, VELOC_Mem_Conf.tag, VELOC_Mem_Exec.globalComm);
        }
        else {
            VELOC_Mem_Exec.wasLastOffline = 0;
            if (res != VELOC_Mem_SCES) {
                VELOC_Mem_Exec.ckptLvel = VELOC_Mem_REJW - VELOC_Mem_BASE;
            }
            res = VELOC_Mem_Try(VELOC_Mem_PostCkpt(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, VELOC_Mem_Topo.groupID, -1, 1), "postprocess the checkpoint.");
            if (res == VELOC_Mem_SCES) {
                VELOC_Mem_Exec.wasLastOffline = 0;
                VELOC_Mem_Exec.lastCkptLvel = VELOC_Mem_Exec.ckptLvel;
            }
        }
        t3 = MPI_Wtime();
        sprintf(catstr, "%s taken in %.2f sec.", str, t3 - t0);
        sprintf(str, "%s (Wt:%.2fs, Wr:%.2fs, Ps:%.2fs)", catstr, t1 - t0, t2 - t1, t3 - t2);
        VELOC_Mem_Print(str, VELOC_Mem_INFO);
        if (res != VELOC_Mem_NSCS) {
            res = VELOC_Mem_DONE;
            if (ckptFirst && VELOC_Mem_Topo.splitRank == 0) {
                VELOC_Mem_Try(VELOC_Mem_UpdateConf(&VELOC_Mem_Conf, &VELOC_Mem_Exec, 1), "update configuration file.");
            }
        }
        else {
            res = VELOC_Mem_NSCS;
        }
    }
    return res;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It loads the checkpoint data.
    @return     integer         VELOC_Mem_SCES if successful.

    This function loads the checkpoint data from the checkpoint file and
    it updates some basic checkpoint information.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Recover()
{
    char fn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    FILE* fd;
    int i;
    sprintf(fn, "%s/%s", VELOC_Mem_Ckpt[VELOC_Mem_Exec.ckptLvel].dir, VELOC_Mem_Exec.meta[VELOC_Mem_Exec.ckptLvel].ckptFile);
    printf("level %d, fn==%s/%s\n", VELOC_Mem_Exec.ckptLvel, VELOC_Mem_Ckpt[VELOC_Mem_Exec.ckptLvel].dir, VELOC_Mem_Exec.meta[VELOC_Mem_Exec.ckptLvel].ckptFile);
    sprintf(str, "Trying to load VELOC_Mem checkpoint file (%s)...", fn);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    fd = fopen(fn, "rb");
    if (fd == NULL) {
        VELOC_Mem_Print("Could not open VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
        return VELOC_Mem_NSCS;
    }
    for (i = 0; i < VELOC_Mem_Exec.nbVar; i++) {
        size_t bytes = fread(VELOC_Mem_Data[i].ptr, 1, VELOC_Mem_Data[i].size, fd);
        if (ferror(fd)) {
            VELOC_Mem_Print("Could not read VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
            fclose(fd);
            return VELOC_Mem_NSCS;
        }
    }
    if (fclose(fd) != 0) {
        VELOC_Mem_Print("Could not close VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
        return VELOC_Mem_NSCS;
    }
    VELOC_Mem_Exec.reco = 0;
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Takes an VELOC_Mem snapshot or recovers the data if it is a restart.
    @return     integer         VELOC_Mem_SCES if successful.

    This function loads the checkpoint data from the checkpoint file in case
    of restart. Otherwise, it checks if the current iteration requires
    checkpointing, if it does it checks which checkpoint level, write the
    data in the files and it communicates with the head of the node to inform
    that a checkpoint has been taken. Checkpoint ID and counters are updated.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Snapshot()
{
    int i, res, level = -1;

    if (VELOC_Mem_Exec.reco) { // If this is a recovery load icheckpoint data
        res = VELOC_Mem_Try(VELOC_Mem_Recover(), "recover the checkpointed data.");
        if (res == VELOC_Mem_NSCS) {
            VELOC_Mem_Print("Impossible to load the checkpoint data.", VELOC_Mem_EROR);
            VELOC_Mem_Clean(&VELOC_Mem_Conf, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, 5, VELOC_Mem_Topo.groupID, VELOC_Mem_Topo.myRank);
            MPI_Abort(MPI_COMM_WORLD, -1);
            MPI_Finalize();
            exit(1);
        }
    }
    else { // If it is a checkpoint test
        res = VELOC_Mem_SCES;
        VELOC_Mem_UpdateIterTime(&VELOC_Mem_Exec);
        if (VELOC_Mem_Exec.ckptNext == VELOC_Mem_Exec.ckptIcnt) { // If it is time to check for possible ckpt. (every minute)
            VELOC_Mem_Print("Checking if it is time to checkpoint.", VELOC_Mem_DBUG);
            if (VELOC_Mem_Exec.globMeanIter > 60) {
                VELOC_Mem_Exec.minuteCnt = VELOC_Mem_Exec.totalIterTime/60;
            }
            else {
                VELOC_Mem_Exec.minuteCnt++; // Increment minute counter
            }
            for (i = 1; i < 5; i++) { // Check ckpt. level
                if (VELOC_Mem_Ckpt[i].ckptIntv > 0 && VELOC_Mem_Exec.minuteCnt/(VELOC_Mem_Ckpt[i].ckptCnt*VELOC_Mem_Ckpt[i].ckptIntv)) {
                    level = i;
                    VELOC_Mem_Ckpt[i].ckptCnt++;
                }
            }
            if (level != -1) {
                res = VELOC_Mem_Try(VELOC_Mem_Checkpoint(VELOC_Mem_Exec.ckptCnt, level), "take checkpoint.");
                if (res == VELOC_Mem_DONE) {
                    VELOC_Mem_Exec.ckptCnt++;
                }
            }
            VELOC_Mem_Exec.ckptLast = VELOC_Mem_Exec.ckptNext;
            VELOC_Mem_Exec.ckptNext = VELOC_Mem_Exec.ckptNext + VELOC_Mem_Exec.ckptIntv;
            VELOC_Mem_Exec.iterTime = MPI_Wtime(); // Reset iteration duration timer
        }
    }
    return res;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It closes VELOC_Mem properly on the application processes.
    @return     integer         VELOC_Mem_SCES if successful.

    This function notifies the VELOC_Mem processes that the execution is over, frees
    some data structures and it closes. If this function is not called on the
    application processes the VELOC_Mem processes will never finish (deadlock).

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Finalize()
{
    int isCkpt;

    if (VELOC_Mem_Topo.amIaHead) {
        VELOC_Mem_FreeMeta(&VELOC_Mem_Exec);
        MPI_Barrier(VELOC_Mem_Exec.globalComm);
        MPI_Finalize();
        exit(0);
    }

    MPI_Reduce( &VELOC_Mem_Exec.ckptID, &isCkpt, 1, MPI_INT, MPI_SUM, 0, VELOC_Mem_COMM_WORLD );
    // Not VELOC_Mem_Topo.amIaHead
    int buff = VELOC_Mem_ENDW;
    MPI_Status status;

    // If there is remaining work to do for last checkpoint
    if (VELOC_Mem_Exec.wasLastOffline == 1) {
        MPI_Recv(&buff, 1, MPI_INT, VELOC_Mem_Topo.headRank, VELOC_Mem_Conf.tag, VELOC_Mem_Exec.globalComm, &status);
        if (buff != VELOC_Mem_NSCS) {
            VELOC_Mem_Exec.ckptLvel = buff;
            VELOC_Mem_Exec.wasLastOffline = 1;
            VELOC_Mem_Exec.lastCkptLvel = VELOC_Mem_Exec.ckptLvel;
        }
    }
    buff = VELOC_Mem_ENDW;

    // Send notice to the head to stop listening
    if (VELOC_Mem_Topo.nbHeads == 1) {
        MPI_Send(&buff, 1, MPI_INT, VELOC_Mem_Topo.headRank, VELOC_Mem_Conf.tag, VELOC_Mem_Exec.globalComm);
    }

    // If we need to keep the last checkpoint
    if (VELOC_Mem_Conf.saveLastCkpt && VELOC_Mem_Exec.ckptID > 0) {
        if (VELOC_Mem_Exec.lastCkptLvel != 4) {
            VELOC_Mem_Try(VELOC_Mem_Flush(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, VELOC_Mem_Exec.lastCkptLvel), "save the last ckpt. in the PFS.");
            MPI_Barrier(VELOC_Mem_COMM_WORLD);
            if (VELOC_Mem_Topo.splitRank == 0) {
                if (access(VELOC_Mem_Ckpt[4].dir, 0) == 0) {
                    VELOC_Mem_RmDir(VELOC_Mem_Ckpt[4].dir, 1);
                }
                if (access(VELOC_Mem_Ckpt[4].metaDir, 0) == 0) {
                    VELOC_Mem_RmDir(VELOC_Mem_Ckpt[4].metaDir, 1);
                }
                if (rename(VELOC_Mem_Ckpt[VELOC_Mem_Exec.lastCkptLvel].metaDir, VELOC_Mem_Ckpt[4].metaDir) == -1) {
                    VELOC_Mem_Print("cannot save last ckpt. metaDir", VELOC_Mem_EROR);
                }
                if (rename(VELOC_Mem_Conf.gTmpDir, VELOC_Mem_Ckpt[4].dir) == -1) {
                    VELOC_Mem_Print("cannot save last ckpt. dir", VELOC_Mem_EROR);
                }
            }
        }
        if (VELOC_Mem_Topo.splitRank == 0) {
            VELOC_Mem_Try(VELOC_Mem_UpdateConf(&VELOC_Mem_Conf, &VELOC_Mem_Exec, 2), "update configuration file to 2.");
        }
        buff = 6; // For cleaning only local storage
    }
    else {
        if (VELOC_Mem_Conf.saveLastCkpt && !isCkpt) {
            VELOC_Mem_Print("No ckpt. to keep.", VELOC_Mem_INFO);
        }
        if (VELOC_Mem_Topo.splitRank == 0) {
            VELOC_Mem_Try(VELOC_Mem_UpdateConf(&VELOC_Mem_Conf, &VELOC_Mem_Exec, 0), "update configuration file to 0.");
        }
        buff = 5; // For cleaning everything
    }
    VELOC_Mem_FreeMeta(&VELOC_Mem_Exec);
    MPI_Barrier(VELOC_Mem_Exec.globalComm);
    VELOC_Mem_Try(VELOC_Mem_Clean(&VELOC_Mem_Conf, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, buff, VELOC_Mem_Topo.groupID, VELOC_Mem_Topo.myRank), "do final clean.");
    VELOC_Mem_Print("VELOC_Mem has been finalized.", VELOC_Mem_INFO);
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Prints VELOC_Mem messages.
    @param      msg             Message to print.
    @param      priority        Priority of the message to be printed.
    @return     void

    This function prints messages depending on their priority and the
    verbosity level set by the user. DEBUG messages are printed by all
    processes with their rank. INFO messages are printed by one process.
    ERROR messages are printed with errno.

 **/
/*-------------------------------------------------------------------------*/
void VELOC_Mem_Print(char* msg, int priority)
{
    if (priority >= VELOC_Mem_Conf.verbosity) {
        if (msg != NULL) {
            switch (priority) {
                case VELOC_Mem_EROR:
                    fprintf(stderr, "[ " RED "VELOC_Mem Error - %06d" RESET " ] : %s : %s \n", VELOC_Mem_Topo.myRank, msg, strerror(errno));
                    break;
                case VELOC_Mem_WARN:
                    fprintf(stdout, "[ " ORG "VELOC_Mem Warning %06d" RESET " ] : %s \n", VELOC_Mem_Topo.myRank, msg);
                    break;
                case VELOC_Mem_INFO:
                    if (VELOC_Mem_Topo.splitRank == 0) {
                        fprintf(stdout, "[ " GRN "VELOC_Mem  Information" RESET " ] : %s \n", msg);
                    }
                    break;
                case VELOC_Mem_DBUG:
                    fprintf(stdout, "[VELOC_Mem Debug - %06d] : %s \n", VELOC_Mem_Topo.myRank, msg);
                    break;
                default:
                    break;
            }
        }
    }
    fflush(stdout);
}
