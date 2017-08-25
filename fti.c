/**
 *  @file   api.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   December, 2013
 *  @brief  API functions for the FTI library.
 */

#include "interface.h"
#include "scr.h"

/** General configuration information used by FTI.                         */
static FTIT_configuration FTI_Conf;

/** Checkpoint information for all levels of checkpoint.                   */
static FTIT_checkpoint FTI_Ckpt[5];

/** Dynamic information for this execution.                                */
static FTIT_execution FTI_Exec;

/** Topology of the system.                                                */
static FTIT_topology FTI_Topo;

/** Array of datasets and all their internal information.                  */
static FTIT_dataset FTI_Data[FTI_BUFS];

/** MPI communicator that splits the global one into app and FTI appart.   */
MPI_Comm FTI_COMM_WORLD;

/** FTI data type for chars.                                               */
FTIT_type FTI_CHAR;
/** FTI data type for short integers.                                      */
FTIT_type FTI_SHRT;
/** FTI data type for integers.                                            */
FTIT_type FTI_INTG;
/** FTI data type for long integers.                                       */
FTIT_type FTI_LONG;
/** FTI data type for unsigned chars.                                      */
FTIT_type FTI_UCHR;
/** FTI data type for unsigned short integers.                             */
FTIT_type FTI_USHT;
/** FTI data type for unsigned integers.                                   */
FTIT_type FTI_UINT;
/** FTI data type for unsigned long integers.                              */
FTIT_type FTI_ULNG;
/** FTI data type for single floating point.                               */
FTIT_type FTI_SFLT;
/** FTI data type for double floating point.                               */
FTIT_type FTI_DBLE;
/** FTI data type for long doble floating point.                           */
FTIT_type FTI_LDBE;


/*-------------------------------------------------------------------------*/
/**
    @brief      It aborts the application.

    This function aborts the application after cleaning the file system.

 **/
/*-------------------------------------------------------------------------*/
void FTI_Abort()
{
    MPI_Abort(MPI_COMM_WORLD, -1);
    MPI_Finalize();
    exit(1);
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Prints FTI messages.
    @param      msg             Message to print.
    @param      priority        Priority of the message to be printed.
    @return     void

    This function prints messages depending on their priority and the
    verbosity level set by the user. DEBUG messages are printed by all
    processes with their rank. INFO messages are printed by one process.
    ERROR messages are printed with errno.

 **/
/*-------------------------------------------------------------------------*/
void FTI_Print(char* msg, int priority)
{
    if (priority >= FTI_Conf.verbosity) {
        if (msg != NULL) {
            switch (priority) {
                case FTI_EROR:
                    fprintf(stderr, "[FTI Error - %06d] : %s : %s \n", FTI_Topo.myRank, msg, strerror(errno));
                    break;
                case FTI_WARN:
                    fprintf(stdout, "[FTI Warning %06d] : %s \n", FTI_Topo.myRank, msg);
                    break;
                case FTI_INFO:
                    if (FTI_Topo.splitRank == 0)
                        fprintf(stdout, "[ FTI  Information ] : %s \n", msg);
                    break;
                case FTI_DBUG:
                    fprintf(stdout, "[FTI Debug - %06d] : %s \n", FTI_Topo.myRank, msg);
                    break;
                default:
                    break;
            }
        }
    }
    fflush(stdout);
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Receive the return code of a function and print a message.
    @param      result          Result to check.
    @param      message         Message to print.
    @return     integer         The same result as passed in parameter.

    This function checks the result from a function and then decides to
    print the message either as a debug message or as a warning.

 **/
/*-------------------------------------------------------------------------*/
int FTI_Try(int result, char* message)
{
    char str[FTI_BUFS];
    if (result == FTI_SCES || result == FTI_DONE) {
        sprintf(str, "FTI succeeded to %s", message);
        FTI_Print(str, FTI_DBUG);
    }
    else {
        sprintf(str, "FTI failed to %s", message);
        FTI_Print(str, FTI_WARN);
        sprintf(str, "Error => %s", strerror(errno));
        FTI_Print(str, FTI_WARN);
    }
    return result;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Initializes FTI.
    @param      configFile      FTI configuration file.
    @param      globalComm      Main MPI communicator of the application.
    @return     integer         FTI_SCES if successful.

    This function initialize the FTI context and prepare the heads to wait
    for checkpoints. FTI processes should never get out of this function. In
    case of restart, checkpoint files should be recovered and in place at the
    end of this function.

 **/
/*-------------------------------------------------------------------------*/
int FTI_Init(char* configFile, MPI_Comm globalComm)
{
    SCR_Init();

    //assert(globalComm == MPI_COMM_WORLD);

    FTI_Exec.globalComm = globalComm;
    MPI_Comm_rank(FTI_Exec.globalComm, &FTI_Topo.myRank);
    MPI_Comm_size(FTI_Exec.globalComm, &FTI_Topo.nbProc);
    FTI_Topo.splitRank = FTI_Topo.myRank;

    MPI_Comm_dup(MPI_COMM_WORLD, &FTI_COMM_WORLD);

    FTI_Conf.verbosity = FTI_EROR;
    FTI_Exec.nbType = 0;

    FTI_Try(FTI_InitBasicTypes(FTI_Data), "create the basic data types.");

    // check to see if we're restarting
    int have_restart;
    char name[SCR_MAX_FILENAME];
    SCR_Have_restart(&have_restart, name);
    if (have_restart) {
        // got a checkpoint available, set our recovery flag
        FTI_Exec.reco = 1;
    }

    FTI_Print("FTI has been initialized.", FTI_INFO);
    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It returns the current status of the recovery flag.
    @return     integer         FTI_Exec.reco

    This function returns the current status of the recovery flag.

 **/
/*-------------------------------------------------------------------------*/
int FTI_Status()
{
    return FTI_Exec.reco;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It initializes a data type.
    @param      type            The data type to be intialized.
    @param      size            The size of the data type to be intialized.
    @return     integer         FTI_SCES if successful.

    This function initalizes a data type. the only information needed is the
    size of the data type, the rest is black box for FTI.

 **/
/*-------------------------------------------------------------------------*/
int FTI_InitType(FTIT_type* type, int size)
{
    type->id = FTI_Exec.nbType;
    type->size = size;
    FTI_Exec.nbType = FTI_Exec.nbType + 1;
    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It creates the basic datatypes and the dataset array.
    @param      FTI_Data        Dataset array.
    @return     integer         FTI_SCES if successful.

    This function creates the basic data types using FTIT_Type.

 **/
/*-------------------------------------------------------------------------*/
int FTI_InitBasicTypes(FTIT_dataset* FTI_Data)
{
    int i;
    for (i = 0; i < FTI_BUFS; i++) {
        FTI_Data[i].id = -1;
    }
    FTI_InitType(&FTI_CHAR, sizeof(char));
    FTI_InitType(&FTI_SHRT, sizeof(short));
    FTI_InitType(&FTI_INTG, sizeof(int));
    FTI_InitType(&FTI_LONG, sizeof(long));
    FTI_InitType(&FTI_UCHR, sizeof(unsigned char));
    FTI_InitType(&FTI_USHT, sizeof(unsigned short));
    FTI_InitType(&FTI_UINT, sizeof(unsigned int));
    FTI_InitType(&FTI_ULNG, sizeof(unsigned long));
    FTI_InitType(&FTI_SFLT, sizeof(float));
    FTI_InitType(&FTI_DBLE, sizeof(double));
    FTI_InitType(&FTI_LDBE, sizeof(long double));
    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It sets/resets the pointer and type to a protected variable.
    @param      id              ID for searches and update.
    @param      ptr             Pointer to the data structure.
    @param      count           Number of elements in the data structure.
    @param      type            Type of elements in the data structure.
    @return     integer         FTI_SCES if successful.

    This function stores a pointer to a data structure, its size, its ID,
    its number of elements and the type of the elements. This list of
    structures is the data that will be stored during a checkpoint and
    loaded during a recovery. It resets the pointer to a data structure,
    its size, its number of elements and the type of the elements if the
    dataset was already previously registered.

 **/
/*-------------------------------------------------------------------------*/
int FTI_Protect(int id, void* ptr, long count, FTIT_type type)
{
    int i, prevSize, updated = 0;
    char str[FTI_BUFS];
    float ckptSize;
    for (i = 0; i < FTI_BUFS; i++) {
        if (id == FTI_Data[i].id) {
            prevSize = FTI_Data[i].size;
            FTI_Data[i].ptr = ptr;
            FTI_Data[i].count = count;
            FTI_Data[i].type = type;
            FTI_Data[i].eleSize = type.size;
            FTI_Data[i].size = type.size * count;
            FTI_Exec.ckptSize = FTI_Exec.ckptSize + (type.size * count) - prevSize;
            updated = 1;
        }
    }
    if (updated) {
        ckptSize = FTI_Exec.ckptSize / (1024.0 * 1024.0);
        sprintf(str, "Variable ID %d reseted. Current ckpt. size per rank is %.2fMB.", id, ckptSize);
        FTI_Print(str, FTI_DBUG);
    }
    else {
        if (FTI_Exec.nbVar >= FTI_BUFS) {
            FTI_Print("Too many variables registered.", FTI_EROR);
            MPI_Abort(MPI_COMM_WORLD, -1);
            MPI_Finalize();
            exit(1);
        }
        FTI_Data[FTI_Exec.nbVar].id = id;
        FTI_Data[FTI_Exec.nbVar].ptr = ptr;
        FTI_Data[FTI_Exec.nbVar].count = count;
        FTI_Data[FTI_Exec.nbVar].type = type;
        FTI_Data[FTI_Exec.nbVar].eleSize = type.size;
        FTI_Data[FTI_Exec.nbVar].size = type.size * count;
        FTI_Exec.nbVar = FTI_Exec.nbVar + 1;
        FTI_Exec.ckptSize = FTI_Exec.ckptSize + (type.size * count);
        ckptSize = FTI_Exec.ckptSize / (1024.0 * 1024.0);
        sprintf(str, "Variable ID %d to protect. Current ckpt. size per rank is %.2fMB.", id, ckptSize);
        FTI_Print(str, FTI_INFO);
    }
    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Takes an FTI snapshot or recover the data if it is a restart.
    @return     integer         FTI_SCES if successful.

    This function loads the checkpoint data from the checkpoint file in case
    of restart. Otherwise, it checks if the current iteration requires
    checkpointing, if it does it checks which checkpoint level, write the
    data in the files and it communicates with the head of the node to inform
    that a checkpoint has been taken. Checkpoint ID and counters are updated.

 **/
/*-------------------------------------------------------------------------*/
int FTI_Snapshot()
{
    int res, level = -1;

    if (FTI_Exec.reco) { // If this is a recovery load icheckpoint data
        res = FTI_Try(FTI_Recover(), "recover the checkpointed data.");
        if (res == FTI_NSCS) {
            FTI_Print("Impossible to load the checkpoint data.", FTI_EROR);
            MPI_Abort(MPI_COMM_WORLD, -1);
            MPI_Finalize();
            exit(1);
        }
    }
    else { // If it is a checkpoint test
        res = FTI_SCES;
        int flag;
        SCR_Need_checkpoint(&flag);
        if (flag) {
            res = FTI_Try(FTI_Checkpoint(-1, level), "take checkpoint.");
        }
    }

    return res;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It loads the checkpoint data.
    @return     integer         FTI_SCES if successful.

    This function loads the checkpoint data from the checkpoint file and
    it updates some basic checkpoint information.

 **/
/*-------------------------------------------------------------------------*/
int FTI_Recover()
{
    char fn[FTI_BUFS], str[FTI_BUFS];
    FILE* fd;
    int i;

    /* open restart phase */
    char ckpt_name[SCR_MAX_FILENAME];
    SCR_Start_restart(ckpt_name);

    /* build file name */
    snprintf(fn, FTI_BUFS, "Ckpt-Rank%d.fti", FTI_Topo.myRank);

    /* get path to open our file */
    char fn_scr[SCR_MAX_FILENAME];
    SCR_Route_file(fn, fn_scr);

    sprintf(str, "Trying to load FTI checkpoint file (%s)...", fn_scr);
    FTI_Print(str, FTI_DBUG);

    fd = fopen(fn_scr, "rb");
    if (fd == NULL) {
        FTI_Print("Could not open FTI checkpoint file.", FTI_EROR);
        return FTI_NSCS;
    }

    for (i = 0; i < FTI_Exec.nbVar; i++) {
        size_t bytes = fread(FTI_Data[i].ptr, 1, FTI_Data[i].size, fd);
        if (ferror(fd)) {
            FTI_Print("Could not read FTI checkpoint file.", FTI_EROR);
            fclose(fd);
            return FTI_NSCS;
        }
    }

    if (fclose(fd) != 0) {
        FTI_Print("Could not close FTI checkpoint file.", FTI_EROR);
        return FTI_NSCS;
    }

    /* close restart phase, assume success in reading if we got here,
     * those procs that fail reading will return early and abort the job */
    SCR_Complete_restart(1);

    FTI_Exec.reco = 0;

    return FTI_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It takes the checkpoint and triggers the post-ckpt. work.
    @param      id              Checkpoint ID.
    @param      level           Checkpoint level.
    @return     integer         FTI_SCES if successfull.

    This function starts by blocking on a receive if the previous ckpt. was
    offline. Then, it updates the ckpt. information. It writes down the ckpt.
    data, creates the metadata and the post-processing work. This function
    is complementary with the FTI_Listen function in terms of communications.

 **/
/*-------------------------------------------------------------------------*/
int FTI_Checkpoint(int id, int level)
{
    int res = FTI_NSCS, value;
    double t0, t1, t2, t3;
    char str[FTI_BUFS];
    char catstr[FTI_BUFS];

    /* open the checkpoint phase */
    SCR_Start_checkpoint();

    t1 = MPI_Wtime();

    res = FTI_Try(FTI_WriteCkpt(&FTI_Conf, &FTI_Exec, &FTI_Topo, FTI_Ckpt, FTI_Data), "write the checkpoint.");

    t2 = MPI_Wtime();

    /* close the checkpoint phase, indicate whether this proc succeeded */
    SCR_Complete_checkpoint(res == FTI_SCES);

    t3 = MPI_Wtime();
    sprintf(catstr, "%s taken in %.2f sec.", str, t3 - t0);
    sprintf(str, "%s (Wt:%.2fs, Wr:%.2fs, Ps:%.2fs)", catstr, t1 - t0, t2 - t1, t3 - t2);
    FTI_Print(str, FTI_INFO);
    if (res != FTI_NSCS)
        res = FTI_DONE;
    else
        res = FTI_NSCS;

    return res;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It writes the checkpoint data in the target file.
    @param      FTI_Data        Dataset array.
    @return     integer         FTI_SCES if successful.

    This function checks whether the checkpoint needs to be local or remote,
    opens the target file and write dataset per dataset, the checkpoint data,
    it finally flushes and closes the checkpoint file.

 **/
/*-------------------------------------------------------------------------*/
int FTI_WriteCkpt(FTIT_configuration* FTI_Conf, FTIT_execution* FTI_Exec,
                  FTIT_topology* FTI_Topo, FTIT_checkpoint* FTI_Ckpt,
                  FTIT_dataset* FTI_Data)

{
    int i, res;
    FILE* fd;
    char fn[FTI_BUFS], str[FTI_BUFS];

    double tt = MPI_Wtime();

    snprintf(fn, FTI_BUFS, "Ckpt-Rank%d.fti", FTI_Topo->myRank);

    char fn_scr[SCR_MAX_FILENAME];
    SCR_Route_file(fn, fn_scr);

    fd = fopen(fn_scr, "wb");
    if (fd == NULL) {
        FTI_Print("FTI checkpoint file could not be opened.", FTI_EROR);
        return FTI_NSCS;
    }

    for (i = 0; i < FTI_Exec->nbVar; i++) {
        if (fwrite(FTI_Data[i].ptr, FTI_Data[i].eleSize, FTI_Data[i].count, fd) != FTI_Data[i].count) {
            sprintf(str, "Dataset #%d could not be written.", FTI_Data[i].id);
            FTI_Print(str, FTI_EROR);
            fclose(fd);
            return FTI_NSCS;
        }
    }

    if (fflush(fd) != 0) {
        FTI_Print("FTI checkpoint file could not be flushed.", FTI_EROR);
        fclose(fd);
        return FTI_NSCS;
    }

    if (fclose(fd) != 0) {
        FTI_Print("FTI checkpoint file could not be flushed.", FTI_EROR);
        return FTI_NSCS;
    }

    sprintf(str, "Time writing checkpoint file : %f seconds.", MPI_Wtime() - tt);
    FTI_Print(str, FTI_DBUG);

    return res;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It closes FTI properly on the application processes.
    @return     integer         FTI_SCES if successful.

    This function notify the FTI processes that the execution is over, frees
    some data structures and it closes. If this function is not called on the
    application processes the FTI processes will never finish (deadlock).

 **/
/*-------------------------------------------------------------------------*/
int FTI_Finalize()
{
    MPI_Comm_free(&FTI_COMM_WORLD);
    SCR_Finalize();
    return FTI_SCES;
}
