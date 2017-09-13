#include <stdlib.h>
#include <stdio.h>

#include "veloc.h"
#include "mpi.h"
#include "vmem/interface.h"
#include "scr.h"


/** General configuration information used by VELOC_Mem.                         */
static VELOCT_configuration VELOC_Mem_Conf;

/** Checkpoint information for all levels of checkpoint.                   */
static VELOCT_checkpoint VELOC_Mem_Ckpt[5];

/** Dynamic information for this execution.                                */
VELOCT_execution VELOC_Mem_Exec;

/** Topology of the system.                                                */
static VELOCT_topology VELOC_Mem_Topo;

/** Array of datasets and all their internal information.                  */
static VELOCT_dataset VELOC_Mem_Data[VELOC_Mem_BUFS];

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


/** Standard size of buffer and mas node size.                             */
#define VELOC_BUFS 256

// initialize restart flag to assume we're not restarting
static int g_recovery = 0;

// checkpoint counter so we can create unique veloc directories for each one
static int g_checkpoint_id = 0;

// buffer to hold name of checkpoint (and checkpoint directory)
static char g_checkpoint_dir[SCR_MAX_FILENAME];

// our global rank
static int g_rank = -1;

// current size of checkpoint in bytes
static unsigned int g_ckptSize = 0;

typedef enum {
    VELOC_STATE_UNINIT,
    VELOC_STATE_INIT,
    VELOC_STATE_RESTART,
    VELOC_STATE_CHECKPOINT
} VELOC_STATE;

VELOC_STATE g_veloc_state = VELOC_STATE_UNINIT;

int VELOC_InitBasicTypes(VELOCT_dataset* VELOC_Data)
{
    int i;
    for (i = 0; i < VELOC_BUFS; i++) {
        VELOC_Data[i].id = -1;
    }

    VELOC_Mem_type(&VELOC_CHAR, sizeof(char));
    VELOC_Mem_type(&VELOC_SHRT, sizeof(short));
    VELOC_Mem_type(&VELOC_INTG, sizeof(int));
    VELOC_Mem_type(&VELOC_LONG, sizeof(long));
    VELOC_Mem_type(&VELOC_UCHR, sizeof(unsigned char));
    VELOC_Mem_type(&VELOC_USHT, sizeof(unsigned short));
    VELOC_Mem_type(&VELOC_UINT, sizeof(unsigned int));
    VELOC_Mem_type(&VELOC_ULNG, sizeof(unsigned long));
    VELOC_Mem_type(&VELOC_SFLT, sizeof(float));
    VELOC_Mem_type(&VELOC_DBLE, sizeof(double));
    VELOC_Mem_type(&VELOC_LDBE, sizeof(long double));

    return VELOC_SUCCESS;
}

/**************************
 * Init / Finalize
 *************************/

/*-------------------------------------------------------------------------*/
/**
    @brief      Initializes VELOC_Mem.
    @param      configFile      VELOC_Mem configuration file.
    @param      globalComm      Main MPI communicator of the application.
    @return     integer         VELOC_SUCCESS if successful.

    This function initializes the VELOC_Mem context and prepares the heads to wait
    for checkpoints. VELOC_Mem processes should never get out of this function. In
    case of a restart, checkpoint files should be recovered and in place at the
    end of this function.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_init(char* configFile, MPI_Comm globalComm)
{
    VELOC_Mem_Exec.globalComm = globalComm;
    MPI_Comm_rank(VELOC_Mem_Exec.globalComm, &VELOC_Mem_Topo.myRank);
    MPI_Comm_size(VELOC_Mem_Exec.globalComm, &VELOC_Mem_Topo.nbProc);
    snprintf(VELOC_Mem_Conf.cfgFile, VELOC_Mem_BUFS, "%s", configFile);
    VELOC_Mem_Conf.verbosity = 1;
    VELOC_Mem_COMM_WORLD = globalComm; // Temporary before building topology
    VELOC_Mem_Topo.splitRank = VELOC_Mem_Topo.myRank; // Temporary before building topology
    int res = VELOC_Mem_Try(VELOC_Mem_LoadConf(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt), "load configuration.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Abort();
    }
    res = VELOC_Mem_Try(VELOC_Mem_Topology(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo), "build topology.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Abort();
    }
    VELOC_Mem_Try(VELOC_Mem_initBasicTypes(VELOC_Mem_Data), "create the basic data types.");
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
            res = VELOC_Mem_Try(VELOC_Mem_recoverFiles(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt), "recover the checkpoint files.");
            if (res != VELOC_SUCCESS) {
                VELOC_Mem_Abort();
            }
        }
        res = 0;
        while (res != VELOC_Mem_ENDW) {
            res = VELOC_Mem_Listen(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt);
        }
        VELOC_Mem_print("Head stopped listening.", VELOC_Mem_DBUG);
        VELOC_Mem_finalize();
    }
    else { // If I am an application process
        if (VELOC_Mem_Exec.reco) {
            res = VELOC_Mem_Try(VELOC_Mem_recoverFiles(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt), "recover the checkpoint files.");
            if (res != VELOC_SUCCESS) {
                VELOC_Mem_Abort();
            }
            VELOC_Mem_Exec.ckptCnt = VELOC_Mem_Exec.ckptID;
            VELOC_Mem_Exec.ckptCnt++;
        }
    }
    VELOC_Mem_print("VELOC_Mem has been initialized.", VELOC_Mem_INFO);
    return VELOC_SUCCESS;
}

int VELOC_Init(char* configFile)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_UNINIT) {
        // ERROR!
    }
    g_veloc_state = VELOC_STATE_INIT;

    // TODO: pass config file to SCR
    SCR_Init();

    // create predefined VELOC types
    VELOC_InitBasicTypes(VELOC_Mem_Data);

    // get our rank
    MPI_Comm_rank(MPI_COMM_WORLD, &g_rank);

    // check to see if we're restarting
    int have_restart;
    char name[SCR_MAX_FILENAME];
    SCR_Have_restart(&have_restart, name);
    if (have_restart) {
        // got a checkpoint available, set our recovery flag
        g_recovery = 1;
    }

	VELOC_Mem_init(configFile, MPI_COMM_WORLD);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It closes VELOC_Mem properly on the application processes.
    @return     integer         VELOC_SUCCESS if successful.

    This function notifies the VELOC_Mem processes that the execution is over, frees
    some data structures and it closes. If this function is not called on the
    application processes the VELOC_Mem processes will never finish (deadlock).

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_finalize()
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
                    VELOC_Mem_print("cannot save last ckpt. metaDir", VELOC_Mem_EROR);
                }
                if (rename(VELOC_Mem_Conf.gTmpDir, VELOC_Mem_Ckpt[4].dir) == -1) {
                    VELOC_Mem_print("cannot save last ckpt. dir", VELOC_Mem_EROR);
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
            VELOC_Mem_print("No ckpt. to keep.", VELOC_Mem_INFO);
        }
        if (VELOC_Mem_Topo.splitRank == 0) {
            VELOC_Mem_Try(VELOC_Mem_UpdateConf(&VELOC_Mem_Conf, &VELOC_Mem_Exec, 0), "update configuration file to 0.");
        }
        buff = 5; // For cleaning everything
    }
    VELOC_Mem_FreeMeta(&VELOC_Mem_Exec);
    MPI_Barrier(VELOC_Mem_Exec.globalComm);
    VELOC_Mem_Try(VELOC_Mem_Clean(&VELOC_Mem_Conf, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, buff, VELOC_Mem_Topo.groupID, VELOC_Mem_Topo.myRank), "do final clean.");
    VELOC_Mem_print("VELOC_Mem has been finalized.", VELOC_Mem_INFO);
    return VELOC_SUCCESS;
}

int VELOC_Finalize()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }
    g_veloc_state = VELOC_STATE_UNINIT;

    // shut down the library (flush final checkpoint if needed)
    SCR_Finalize();
    
    VELOC_Mem_finalize();

    return VELOC_SUCCESS;
}

int VELOC_Exit_test(int* flag)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // determine whether the job should exit now
    SCR_Should_exit(flag);

    return VELOC_SUCCESS;
}

/**************************
 * Memory registration
 *************************/

int VELOC_Mem_type(VELOCT_type* type, int size)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // create a new memory datatype
    type->id = VELOC_Mem_Exec.nbType;
    type->size = size;
    VELOC_Mem_Exec.nbType = VELOC_Mem_Exec.nbType + 1;

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It sets/resets the pointer and type to a protected variable.
    @param      id              ID for searches and update.
    @param      ptr             Pointer to the data structure.
    @param      count           Number of elements in the data structure.
    @param      type            Type of elements in the data structure.
    @return     integer         VELOC_SUCCESS if successful.

    This function stores a pointer to a data structure, its size, its ID,
    its number of elements and the type of the elements. This list of
    structures is the data that will be stored during a checkpoint and
    loaded during a recovery. It resets the pointer to a data structure,
    its size, its number of elements and the type of the elements if the
    dataset was already previously registered.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_protect(int id, void* ptr, long count, VELOCT_type type)
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
        VELOC_Mem_print(str, VELOC_Mem_DBUG);
    }
    else {
        if (VELOC_Mem_Exec.nbVar >= VELOC_Mem_BUFS) {
            VELOC_Mem_print("Too many variables registered.", VELOC_Mem_EROR);
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
        VELOC_Mem_print(str, VELOC_Mem_INFO);
    }
    return VELOC_SUCCESS;
}

/**************************
 * File registration
 *************************/

int VELOC_Route_file(const char* name, char* veloc_name)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_RESTART &&
        g_veloc_state != VELOC_STATE_CHECKPOINT)
    {
        // ERROR!
    }

    // get full path we should use to open the file
    SCR_Route_file(name, veloc_name);

    return VELOC_SUCCESS;
}

/**************************
 * Restart routines
 *************************/

int VELOC_Restart_test(int* flag)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // inform caller whether we have a checkpoint to read
    // or whether this is a new run (no checkpoint)
    *flag = g_recovery;

    return VELOC_SUCCESS;
}

int VELOC_Restart_begin()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }
    g_veloc_state = VELOC_STATE_RESTART;

    // enter restart phase, and get name of the checkpoint we're restarting from
    // this name is also used as the directory where we wrote files
    SCR_Start_restart(g_checkpoint_dir);

    // extract id from name
    sscanf(g_checkpoint_dir, "veloc.%d", &g_checkpoint_id);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It reads memory from checkpoint data.
    @return     integer         VELOC_SUCCESS if successful.

    This function loads the checkpoint data from the checkpoint file and
    it updates some basic checkpoint information.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Restart_mem(int recovery_mode, int *id_list, int id_count)
{
    char fn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    FILE* fd;
    int i, status;
    sprintf(fn, "%s/%s", VELOC_Mem_Ckpt[VELOC_Mem_Exec.ckptLvel].dir, VELOC_Mem_Exec.meta[VELOC_Mem_Exec.ckptLvel].ckptFile);
    printf("level %d, fn==%s/%s\n", VELOC_Mem_Exec.ckptLvel, VELOC_Mem_Ckpt[VELOC_Mem_Exec.ckptLvel].dir, VELOC_Mem_Exec.meta[VELOC_Mem_Exec.ckptLvel].ckptFile);
    sprintf(str, "Trying to load VELOC_Mem checkpoint file (%s)...", fn);
    VELOC_Mem_print(str, VELOC_Mem_DBUG);

    fd = fopen(fn, "rb");
    if (fd == NULL) {
        VELOC_Mem_print("Could not open VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
        return VELOC_Mem_NSCS;
    }
    
    if(recovery_mode==VELOC_RECOVER_ALL)
    {
		for (i = 0; i < VELOC_Mem_Exec.nbVar; i++) {
			size_t bytes = fread(VELOC_Mem_Data[i].ptr, 1, VELOC_Mem_Data[i].size, fd);
			if (ferror(fd)) {
				VELOC_Mem_print("Could not read VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
				fclose(fd);
				return VELOC_Mem_NSCS;
			}
		}	
		status = VELOC_SUCCESS;
	}
	else if(recovery_mode==VELOC_RECOVER_REST)
	{
		for (i = 0; i < VELOC_Mem_Exec.nbVar; i++) {
			if(VELOC_SUCCESS != VELOC_Mem_Check_ID_Exist(VELOC_Mem_Data[i].id, id_list, id_count))
			{
				size_t bytes = fread(VELOC_Mem_Data[i].ptr, 1, VELOC_Mem_Data[i].size, fd);
				if (ferror(fd)) {
					VELOC_Mem_print("Could not read VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
					fclose(fd);
					return VELOC_SUCCESS;
				}			
			}
			else
			{
				fseek(fd, VELOC_Mem_Data[i].size, SEEK_CUR);
			}
		}		
		status = VELOC_SUCCESS;
	}
	else if(recovery_mode==VELOC_RECOVER_SOME)
	{
		if(id_count<=0||id_list==NULL)
			status = VELOC_Mem_NSCS;
		else
		{
			for (i = 0; i < VELOC_Mem_Exec.nbVar; i++) {
				if(VELOC_Mem_Check_ID_Exist(VELOC_Mem_Data[i].id, id_list, id_count))
				{
					size_t bytes = fread(VELOC_Mem_Data[i].ptr, 1, VELOC_Mem_Data[i].size, fd);
					if (ferror(fd)) {
						VELOC_Mem_print("Could not read VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
						fclose(fd);
						return VELOC_SUCCESS;
					}			
				}
				else
				{
					fseek(fd, VELOC_Mem_Data[i].size, SEEK_CUR);
				}
			}
			status = VELOC_SUCCESS;			
		}
	}
	else 
	{
		status = VELOC_Mem_NSCS;
	}
    if (fclose(fd) != 0) {
        VELOC_Mem_print("Could not close VELOC_Mem checkpoint file.", VELOC_Mem_EROR);
        status = VELOC_Mem_NSCS;
    }
    if(status == VELOC_Mem_NSCS)
		VELOC_Mem_print("Error occurs in VELOC_Restart_mem() of veloc.c.", VELOC_Mem_EROR);
    VELOC_Mem_Exec.reco = 0;
    return status;
}


int VELOC_Restart_end(int valid)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_RESTART) {
        // ERROR!
    }
    g_veloc_state = VELOC_STATE_INIT;

    // TODO: combiner user valid with memory file valid here
    // complete restart phase
    SCR_Complete_restart(valid);

    // app read in its checkpoint, turn off recovery flag
    g_recovery = 0;

    return VELOC_SUCCESS;
}

/**************************
 * Checkpoint routines
 *************************/

// flag returns 1 if checkpoint should be taken, 0 otherwise
int VELOC_Checkpoint_test(int* flag)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // determine whether app should checkpoint now
    SCR_Need_checkpoint(flag);

    return VELOC_SUCCESS;
}

int VELOC_Checkpoint_begin()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }
    g_veloc_state = VELOC_STATE_CHECKPOINT;

    // bump our checkpoint counter
    g_checkpoint_id++;

    // create a name for our checkpoint
    sprintf(g_checkpoint_dir, "veloc.%d", g_checkpoint_id);

    // open our checkpoint phase
    SCR_Start_output(g_checkpoint_dir, SCR_FLAG_CHECKPOINT);

    return VELOC_SUCCESS;
}

// writes protected memory to file
/*-------------------------------------------------------------------------*/
/**
    @brief      It takes the checkpoint and triggers the post-ckpt. work.
    @param      id              Checkpoint ID.
    @param      level           Checkpoint level.
    @return     integer         VELOC_SUCCESS if successful.

    This function starts by blocking on a receive if the previous ckpt. was
    offline. Then, it updates the ckpt. information. It writes down the ckpt.
    data, creates the metadata and the post-processing work. This function
    is complementary with the VELOC_Mem_Listen function in terms of communications.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Checkpoint_mem(int id, int level)
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
            if (res == VELOC_SUCCESS) {
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
            if (res != VELOC_SUCCESS) {
                value = VELOC_Mem_REJW;
            }
            else {
                value = VELOC_Mem_BASE + VELOC_Mem_Exec.ckptLvel;
            }
            MPI_Send(&value, 1, MPI_INT, VELOC_Mem_Topo.headRank, VELOC_Mem_Conf.tag, VELOC_Mem_Exec.globalComm);
        }
        else {
            VELOC_Mem_Exec.wasLastOffline = 0;
            if (res != VELOC_SUCCESS) {
                VELOC_Mem_Exec.ckptLvel = VELOC_Mem_REJW - VELOC_Mem_BASE;
            }
            res = VELOC_Mem_Try(VELOC_Mem_PostCkpt(&VELOC_Mem_Conf, &VELOC_Mem_Exec, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, VELOC_Mem_Topo.groupID, -1, 1), "postprocess the checkpoint.");
            if (res == VELOC_SUCCESS) {
                VELOC_Mem_Exec.wasLastOffline = 0;
                VELOC_Mem_Exec.lastCkptLvel = VELOC_Mem_Exec.ckptLvel;
            }
        }
        t3 = MPI_Wtime();
        sprintf(catstr, "%s taken in %.2f sec.", str, t3 - t0);
        sprintf(str, "%s (Wt:%.2fs, Wr:%.2fs, Ps:%.2fs)", catstr, t1 - t0, t2 - t1, t3 - t2);
        VELOC_Mem_print(str, VELOC_Mem_INFO);
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

int VELOC_Checkpoint_end(int valid)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_CHECKPOINT) {
        // ERROR!
    }
    g_veloc_state = VELOC_STATE_INIT;

    // TODO: we should track success/failure of Mem_checkpoint

    // mark the end of the checkpoint, indicate whether this process
    // wrote its data successfully or not
    SCR_Complete_output(valid);

    return VELOC_SUCCESS;
}

/**************************
 * convenience functions for existing FTI users
 * (can be implemented fully with above functions)
 ************************/

int VELOC_Mem_save(int id, int level)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // write protected memory to file
    VELOC_Checkpoint_begin();
    int rc = VELOC_Checkpoint_mem(id, level);
    VELOC_Checkpoint_end((rc == VELOC_SUCCESS));

    return VELOC_SUCCESS;
}

int VELOC_Mem_recover(int recovery_mode, int *id_list, int id_count)
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // read protected memory from file
    //VELOC_Restart_begin();
    int rc = VELOC_Restart_mem(recovery_mode, id_list, id_count);
    //VELOC_Restart_end((rc == VELOC_SUCCESS));

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Takes an VELOC_Mem_snapshot or recovers the data if it is a restart.
    @return     integer         VELOC_SUCCESS if successful.

    This function loads the checkpoint data from the checkpoint file in case
    of restart. Otherwise, it checks if the current iteration requires
    checkpointing, if it does it checks which checkpoint level, write the
    data in the files and it communicates with the head of the node to inform
    that a checkpoint has been taken. Checkpoint ID and counters are updated.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_snapshot()
{
    int i, res, level = -1;

    if (VELOC_Mem_Exec.reco) { // If this is a recovery load icheckpoint data
        res = VELOC_Mem_Try(VELOC_Mem_recover(VELOC_RECOVER_ALL, NULL, 0), "recover the checkpointed data.");
        if (res == VELOC_Mem_NSCS) {
            VELOC_Mem_print("Impossible to load the checkpoint data.", VELOC_Mem_EROR);
            VELOC_Mem_Clean(&VELOC_Mem_Conf, &VELOC_Mem_Topo, VELOC_Mem_Ckpt, 5, VELOC_Mem_Topo.groupID, VELOC_Mem_Topo.myRank);
            MPI_Abort(MPI_COMM_WORLD, -1);
            MPI_Finalize();
            exit(1);
        }
    }
    else { // If it is a checkpoint test
        res = VELOC_SUCCESS;
        VELOC_Mem_UpdateIterTime(&VELOC_Mem_Exec);
        if (VELOC_Mem_Exec.ckptNext == VELOC_Mem_Exec.ckptIcnt) { // If it is time to check for possible ckpt. (every minute)
            VELOC_Mem_print("Checking if it is time to checkpoint.", VELOC_Mem_DBUG);
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
                res = VELOC_Mem_Try(VELOC_Mem_save(VELOC_Mem_Exec.ckptCnt, level), "take checkpoint.");
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
    @brief      It returns the current status of the recovery flag.
    @return     integer         VELOC_Mem_Exec.reco.

    This function returns the current status of the recovery flag.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_status()
{
    return VELOC_Mem_Exec.reco;
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
void VELOC_Mem_print(char* msg, int priority)
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
