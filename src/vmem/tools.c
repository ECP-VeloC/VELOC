/**
 *  @file   tools.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   December, 2013
 *  @brief  Utility functions for the VELOC_Mem library.
 */

#include "interface.h"
#include <dirent.h>
#define CHUNK_SIZE 4096    /**< MD5 algorithm chunk size.      */

/*-------------------------------------------------------------------------*/
/**
    @brief      It calculates checksum of the checkpoint file.
    @param      fileName        Filename of the checkpoint.
    @param      checksum        Checksum that is calculated.
    @return     integer         VELOC_SUCCESS if successful.

    This function calculates checksum of the checkpoint file based on
    MD5 algorithm and saves it in checksum.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Checksum(char* fileName, char* checksum)
{
    MD5_CTX mdContext;
    unsigned char data[CHUNK_SIZE];
    unsigned char hash[MD5_DIGEST_LENGTH];
    int bytes;
    char str[VELOC_Mem_BUFS];
    double startTime = MPI_Wtime();
    int i;

    FILE *fd = fopen(fileName, "rb");
    if (fd == NULL) {
        sprintf(str, "VELOC_Mem failed to open file %s to calculate checksum.", fileName);
        VELOC_Mem_print(str, VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, CHUNK_SIZE, fd)) != 0) {
        MD5_Update (&mdContext, data, bytes);
    }
    MD5_Final (hash, &mdContext);

    for(i = 0; i < MD5_DIGEST_LENGTH -1; i++)
        sprintf(&checksum[i], "%02x", hash[i]);
    checksum[i] = '\0'; //to get a proper string

    sprintf(str, "Checksum took %.2f sec.", MPI_Wtime() - startTime);
    VELOC_Mem_print(str, VELOC_Mem_DBUG);

    fclose (fd);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It compares checksum of the checkpoint file.
    @param      fileName        Filename of the checkpoint.
    @param      checksumToCmp   Checksum to compare.
    @return     integer         VELOC_SUCCESS if successful.

    This function calculates checksum of the checkpoint file based on
    MD5 algorithm. It compares calculated hash value with the one saved
    in the file.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_VerifyChecksum(char* fileName, char* checksumToCmp)
{
    MD5_CTX mdContext;
    unsigned char data[CHUNK_SIZE];
    unsigned char hash[MD5_DIGEST_LENGTH];
    char checksum[MD5_DIGEST_LENGTH];   //calculated checksum
    int bytes;
    char str[VELOC_Mem_BUFS];
    int i;

    FILE *fd = fopen(fileName, "rb");
    if (fd == NULL) {
        sprintf(str, "VELOC_Mem failed to open file %s to calculate checksum.", fileName);
        VELOC_Mem_print(str, VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, CHUNK_SIZE, fd)) != 0) {
        MD5_Update (&mdContext, data, bytes);
    }
    MD5_Final (hash, &mdContext);

    for(i = 0; i < MD5_DIGEST_LENGTH -1; i++)
        sprintf(&checksum[i], "%02x", hash[i]);
    checksum[i] = '\0'; //to get a proper string

    if (memcmp(checksum, checksumToCmp, MD5_DIGEST_LENGTH - 1) != 0) {
        sprintf(str, "Checksum do not match. \"%s\" file is corrupted. %s != %s",
            fileName, checksum, checksumToCmp);
        VELOC_Mem_print(str, VELOC_Mem_WARN);
        fclose (fd);

        return VELOC_Mem_NSCS;
    }

    fclose (fd);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It receives the return code of a function and prints a message.
    @param      result          Result to check.
    @param      message         Message to print.
    @return     integer         The same result as passed in parameter.

    This function checks the result from a function and then decides to
    print the message either as a debug message or as a warning.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Try(int result, char* message)
{
    char str[VELOC_Mem_BUFS];
    if (result == VELOC_SUCCESS || result == VELOC_Mem_DONE) {
        sprintf(str, "VELOC_Mem succeeded to %s", message);
        VELOC_Mem_print(str, VELOC_Mem_DBUG);
    }
    else {
        sprintf(str, "VELOC_Mem failed to %s", message);
        VELOC_Mem_print(str, VELOC_Mem_WARN);
        sprintf(str, "Error => %s", strerror(errno));
        VELOC_Mem_print(str, VELOC_Mem_WARN);
    }
    return result;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It mallocs memory for the metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.

    This function mallocs the memory used for the metadata storage.

 **/
/*-------------------------------------------------------------------------*/
void VELOC_Mem_MallocMeta(VELOCT_execution* VELOC_Mem_Exec, VELOCT_topology* VELOC_Mem_Topo)
{
    int i;
    if (VELOC_Mem_Topo->amIaHead) {
        for (i = 0; i < 5; i++) {
            VELOC_Mem_Exec->meta[i].exists = calloc(VELOC_Mem_Topo->nodeSize, sizeof(int));
            VELOC_Mem_Exec->meta[i].maxFs = calloc(VELOC_Mem_Topo->nodeSize, sizeof(long));
            VELOC_Mem_Exec->meta[i].fs = calloc(VELOC_Mem_Topo->nodeSize, sizeof(long));
            VELOC_Mem_Exec->meta[i].pfs = calloc(VELOC_Mem_Topo->nodeSize, sizeof(long));
            VELOC_Mem_Exec->meta[i].ckptFile = calloc(VELOC_Mem_BUFS * VELOC_Mem_Topo->nodeSize, sizeof(char));
        }
    } else {
        for (i = 0; i < 5; i++) {
            VELOC_Mem_Exec->meta[i].exists = calloc(1, sizeof(int));
            VELOC_Mem_Exec->meta[i].maxFs = calloc(1, sizeof(long));
            VELOC_Mem_Exec->meta[i].fs = calloc(1, sizeof(long));
            VELOC_Mem_Exec->meta[i].pfs = calloc(1, sizeof(long));
            VELOC_Mem_Exec->meta[i].ckptFile = calloc(VELOC_Mem_BUFS, sizeof(char));
        }
    }
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It frees memory for the metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.

    This function frees the memory used for the metadata storage.

 **/
/*-------------------------------------------------------------------------*/
void VELOC_Mem_FreeMeta(VELOCT_execution* VELOC_Mem_Exec)
{
    int i;
    for (i = 0; i < 5; i++) {
        free(VELOC_Mem_Exec->meta[i].exists);
        free(VELOC_Mem_Exec->meta[i].maxFs);
        free(VELOC_Mem_Exec->meta[i].fs);
        free(VELOC_Mem_Exec->meta[i].pfs);
        free(VELOC_Mem_Exec->meta[i].ckptFile);
    }
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It creates the basic datatypes and the dataset array.
    @param      VELOC_Mem_Data        Dataset metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function creates the basic data types using VELOC_MemT_Type.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_initBasicTypes(VELOCT_dataset* VELOC_Mem_Data)
{
    int i;
    for (i = 0; i < VELOC_Mem_BUFS; i++) {
        VELOC_Mem_Data[i].id = -1;
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

/*-------------------------------------------------------------------------*/
/**
    @brief      It erases a directory and all its files.
    @param      path            Path to the directory we want to erase.
    @param      flag            Set to 1 to activate.
    @return     integer         VELOC_SUCCESS if successful.

    This function erases a directory and all its files. It focusses on the
    checkpoint directories created by VELOC_Mem so it does NOT handle recursive
    erasing if the given directory includes other directories.

 **/
/*-------------------------------------------------------------------------*/

int VELOC_Mem_RmDir(char path[VELOC_Mem_BUFS], int flag)
{
    if (flag) {
        char buf[VELOC_Mem_BUFS], fn[VELOC_Mem_BUFS], fil[VELOC_Mem_BUFS];
        DIR* dp = NULL;
        struct dirent* ep = NULL;

        sprintf(buf, "Removing directory %s and its files.", path);
        VELOC_Mem_print(buf, VELOC_Mem_DBUG);

        dp = opendir(path);
        if (dp != NULL) {
            while ((ep = readdir(dp)) != NULL) {
                sprintf(fil, "%s", ep->d_name);
                if ((strcmp(fil, ".") != 0) && (strcmp(fil, "..") != 0)) {
                    sprintf(fn, "%s/%s", path, fil);
                    sprintf(buf, "File %s will be removed.", fn);
                    VELOC_Mem_print(buf, VELOC_Mem_DBUG);
                    if (remove(fn) == -1) {
                        if (errno != ENOENT) {
                            VELOC_Mem_print("Error removing target file.", VELOC_Mem_EROR);
                        }
                    }
                }
            }
        }
        else {
            if (errno != ENOENT) {
                VELOC_Mem_print("Error with opendir.", VELOC_Mem_EROR);
            }
        }
        if (dp != NULL) {
            closedir(dp);
        }

        if (remove(path) == -1) {
            if (errno != ENOENT) {
                VELOC_Mem_print("Error removing target directory.", VELOC_Mem_EROR);
            }
        }
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It erases the previous checkpoints and their metadata.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      level           Level of cleaning.
    @param      group           Group ID of the cleaning target process.
    @param      rank            Rank of the cleaning target process.
    @return     integer         VELOC_SUCCESS if successful.

    This function erases previous checkpoint depending on the level of the
    current checkpoint. Level 5 means complete clean up. Level 6 means clean
    up local nodes but keep last checkpoint data and metadata in the PFS.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Clean(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
              VELOCT_checkpoint* VELOC_Mem_Ckpt, int level, int group, int rank)
{
    char buf[VELOC_Mem_BUFS];
    int nodeFlag, globalFlag = !VELOC_Mem_Topo->splitRank;

    nodeFlag = (((!VELOC_Mem_Topo->amIaHead) && ((VELOC_Mem_Topo->nodeRank - VELOC_Mem_Topo->nbHeads) == 0)) || (VELOC_Mem_Topo->amIaHead)) ? 1 : 0;

    if (level == 0) {
        VELOC_Mem_RmDir(VELOC_Mem_Conf->mTmpDir, globalFlag);
        VELOC_Mem_RmDir(VELOC_Mem_Conf->gTmpDir, globalFlag);
        VELOC_Mem_RmDir(VELOC_Mem_Conf->lTmpDir, nodeFlag);
    }

    // Clean last checkpoint level 1
    if (level >= 1) {
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[1].metaDir, globalFlag);
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[1].dir, nodeFlag);
    }

    // Clean last checkpoint level 2
    if (level >= 2) {
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[2].metaDir, globalFlag);
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[2].dir, nodeFlag);
    }

    // Clean last checkpoint level 3
    if (level >= 3) {
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[3].metaDir, globalFlag);
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[3].dir, nodeFlag);
    }

    // Clean last checkpoint level 4
    if (level == 4 || level == 5) {
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[4].metaDir, globalFlag);
        VELOC_Mem_RmDir(VELOC_Mem_Ckpt[4].dir, globalFlag);
        rmdir(VELOC_Mem_Conf->gTmpDir);
    }

    // If it is the very last cleaning and we DO NOT keep the last checkpoint
    if (level == 5) {
        rmdir(VELOC_Mem_Conf->lTmpDir);
        rmdir(VELOC_Mem_Conf->localDir);
        rmdir(VELOC_Mem_Conf->glbalDir);
        snprintf(buf, VELOC_Mem_BUFS, "%s/Topology.velec_mem", VELOC_Mem_Conf->metadDir);
        if (remove(buf) == -1) {
            if (errno != ENOENT) {
                VELOC_Mem_print("Cannot remove Topology.velec_mem", VELOC_Mem_EROR);
            }
        }
        rmdir(VELOC_Mem_Conf->metadDir);
    }

    // If it is the very last cleaning and we DO keep the last checkpoint
    if (level == 6) {
        rmdir(VELOC_Mem_Conf->lTmpDir);
        rmdir(VELOC_Mem_Conf->localDir);
    }

    return VELOC_SUCCESS;
}

int VELOC_Mem_Check_ID_Exist(int targetID, int* varIDList, int varIDCount)
{
	int i = 0;
	for(i=0;i<varIDCount;i++)
	{
		if(varIDList[i] == targetID)
			return 1;
	}
	return VELOC_SUCCESS;
}
