#include <stdlib.h>
#include <stdio.h>

#include "veloc.h"
#include "mpi.h"
#include "veloc_mem.h"
#include "scr.h"

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

static int VeloC_InitBasicTypes(VELOCT_dataset* VELOC_Data)
{
    // initialize our type count
    g_nbType = 0;

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
    veloc_InitBasicTypes(VELOC_Data);

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

	VELOC_Mem_Init(configFile, MPI_COMM_WORLD);

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
    
    VELOC_Mem_Finalize();

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
	VELOC_Mem_InitType(type, size);

    return VELOC_SUCCESS;
}

int VELOC_Mem_protect(int id, void* ptr, long count, VELOCT_type type)
{
    // manage state transition
/*    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // search to see if we already registered this id
    int i, updated = 0;
    for (i = 0; i < VELOC_BUFS; i++) {
        if (id == VELOC_Data[i].id) {
            // found existing variable with this id, update fields

            // subtract current size from total
            g_ckptSize -= VELOC_Data[i].size;

            // set fields to new values
            VELOC_Data[i].ptr     = ptr;
            VELOC_Data[i].count   = count;
            VELOC_Data[i].type    = type;
            VELOC_Data[i].eleSize = type.size;
            VELOC_Data[i].size    = type.size * count;

            // add bytes to total
            g_ckptSize += type.size * count;

            updated = 1;
        }
    }

    float ckptSize;
    if (updated) {
        ckptSize = g_ckptSize / (1024.0 * 1024.0);
        printf("Variable ID %d reset. Current ckpt. size per rank is %.2fMB.\n", id, ckptSize);
    } else {
        // check that user hasn't overflowed total allowed regions
        if (g_nbVar >= VELOC_BUFS) {
            printf("Too many variables registered.\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
            MPI_Finalize();
            exit(1);
        }

        // set fields for this memory region
        VELOC_Data[g_nbVar].id      = id;
        VELOC_Data[g_nbVar].ptr     = ptr;
        VELOC_Data[g_nbVar].count   = count;
        VELOC_Data[g_nbVar].type    = type;
        VELOC_Data[g_nbVar].eleSize = type.size;
        VELOC_Data[g_nbVar].size    = type.size * count;

        // increment region count and add bytes to total size
        g_nbVar++;
        g_ckptSize += type.size * count;

        ckptSize = g_ckptSize / (1024.0 * 1024.0);
        printf("Variable ID %d to protect. Current ckpt. size per rank is %.2fMB.\n", id, ckptSize);
    }*/
    
    

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

// reads protected memory from file
int VELOC_Restart_mem()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_RESTART) {
        // ERROR!
    }

    // build checkpoint file name
    char fn[VELOC_MAX_NAME];
    snprintf(fn, VELOC_MAX_NAME, "%s/mem.%d.veloc", g_checkpoint_dir, g_rank);

    // get SCR path to checkpoint file
    char fn_scr[SCR_MAX_FILENAME];
    SCR_Route_file(fn, fn_scr);

    // open file for reading
    FILE* fd = fopen(fn_scr, "rb");
    if (fd == NULL) {
        printf("Could not open VELOC Mem checkpoint file %s\n", fn_scr);
        return VELOC_FAILURE;
    }

    // read protected memory
    int i;
    for (i = 0; i < g_nbVar; i++) {
        size_t bytes = fread(VELOC_Data[i].ptr, 1, VELOC_Data[i].size, fd);
        if (ferror(fd)) {
            printf("Could not read VELOC checkpoint file %s\n", fn_scr);
            fclose(fd);
            return VELOC_FAILURE;
        }
    }

    // close the file
    if (fclose(fd) != 0) {
        printf("Could not close VELOC checkpoint file %s", fn_scr);
        return VELOC_FAILURE;
    }

    return VELOC_SUCCESS;
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
int VELOC_Checkpoint_mem()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_CHECKPOINT) {
        // ERROR!
    }

    // build checkpoint file name
    char fn[VELOC_MAX_NAME];
    snprintf(fn, VELOC_MAX_NAME, "%s/mem.%d.veloc", g_checkpoint_dir, g_rank);

    // get SCR path to checkpoint file
    char fn_scr[SCR_MAX_FILENAME];
    SCR_Route_file(fn, fn_scr);

    // open checkpoint file
    FILE* fd = fopen(fn_scr, "wb");
    if (fd == NULL) {
        printf("VELOC checkpoint file could not be opened %s", fn_scr);
        return VELOC_FAILURE;
    }

    // write protected memory
    int i;
    for (i = 0; i < g_nbVar; i++) {
        if (fwrite(VELOC_Data[i].ptr, VELOC_Data[i].eleSize, VELOC_Data[i].count, fd) != VELOC_Data[i].count) {
            printf("Dataset #%d could not be written to %s\n", VELOC_Data[i].id, fn_scr);
            fclose(fd);
            return VELOC_FAILURE;
        }
    }

    // flush data to disk
    if (fflush(fd) != 0) {
        printf("VELOC checkpoint file could not be flushed %s\n", fn_scr);
        fclose(fd);
        return VELOC_FAILURE;
    }

    // close the file
    if (fclose(fd) != 0) {
        printf("VELOC checkpoint file could not be flushed %s\n", fn_scr);
        return VELOC_FAILURE;
    }

    return VELOC_SUCCESS;
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

int VELOC_Mem_save()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // write protected memory to file
    VELOC_Checkpoint_begin();
    int rc = VELOC_Checkpoint_mem();
    VELOC_Checkpoint_end((rc == VELOC_SUCCESS));

    return VELOC_SUCCESS;
}

int VELOC_Mem_recover()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // read protected memory from file
    VELOC_Restart_begin();
    int rc = VELOC_Restart_mem();
    VELOC_Restart_end((rc == VELOC_SUCCESS));

    return VELOC_SUCCESS;
}

int VELOC_Mem_snapshot()
{
    // manage state transition
    if (g_veloc_state != VELOC_STATE_INIT) {
        // ERROR!
    }

    // check whether this is a restart
    int have_restart;
    VELOC_Restart_test(&have_restart);
    if (have_restart) {
        // If this is a recovery load checkpoint data
        return VELOC_Mem_recover();
    }

    // otherwise checkpoint if it's time
    int flag;
    VELOC_Checkpoint_test(&flag);
    if (flag) {
        // it's time, take a checkpoint
        return VELOC_Mem_save();
    }

    // not a restart, but don't need to checkpoint yet
    return VELOC_SUCCESS;
}
