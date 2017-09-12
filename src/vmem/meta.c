/**
 *  @file   meta.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   December, 2013
 *  @brief  Metadata functions for the VELOC_Mem library.
 */

#include "interface.h"

/*-------------------------------------------------------------------------*/
/**
    @brief      It gets the checksums from metadata.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      checksum        Pointer to fill the checkpoint checksum.
    @param      ptnerChecksum   Pointer to fill the ptner file checksum.
    @param      rsChecksum      Pointer to fill the RS file checksum.
    @param      group           The group in the node.
    @param      level           The level of the ckpt or 0 if tmp.
    @return     integer         VELOC_SUCCESS if successful.

    This function reads the metadata file created during checkpointing and
    recovers the checkpoint checksum. If there is no RS file, rsChecksum
    string length is 0.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_GetChecksums(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                    VELOCT_checkpoint* VELOC_Mem_Ckpt, char* checksum, char* ptnerChecksum,
                    char* rsChecksum, int group, int level)
{
    dictionary* ini;
    char* checksumTemp;
    char mfn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    if (level == 0) {
        sprintf(mfn, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, group);
    }
    else {
        sprintf(mfn, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Ckpt[level].metaDir, VELOC_Mem_Topo->sectorID, group);
    }

    sprintf(str, "Getting VELOC_Mem metadata file (%s)...", mfn);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    if (access(mfn, R_OK) != 0) {
        VELOC_Mem_Print("VELOC_Mem metadata file NOT accessible.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    ini = iniparser_load(mfn);
    if (ini == NULL) {
        VELOC_Mem_Print("Iniparser failed to parse the metadata file.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    sprintf(str, "%d:Ckpt_checksum", VELOC_Mem_Topo->groupRank);
    checksumTemp = iniparser_getstring(ini, str, NULL);
    strncpy(checksum, checksumTemp, MD5_DIGEST_LENGTH);
    sprintf(str, "%d:Ckpt_checksum", (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize);
    checksumTemp = iniparser_getstring(ini, str, NULL);
    strncpy(ptnerChecksum, checksumTemp, MD5_DIGEST_LENGTH);
    sprintf(str, "%d:RSed_checksum", VELOC_Mem_Topo->groupRank);
    checksumTemp = iniparser_getstring(ini, str, NULL);
    if (checksumTemp != NULL) {
        strncpy(rsChecksum, checksumTemp, MD5_DIGEST_LENGTH);
    } else {
        rsChecksum[0] = '\0';
    }

    iniparser_freedict(ini);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It writes the RSed file checksum to metadata.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      rank            global rank of the process
    @param      checksum        Pointer to the checksum.
    @return     integer         VELOC_SUCCESS if successful.

    This function should be executed only by one process per group. It
    writes the RSed checksum to the metadata file.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_WriteRSedChecksum(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                            VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                             int rank, char* checksum)
{
    char str[VELOC_Mem_BUFS], buf[VELOC_Mem_BUFS], fileName[VELOC_Mem_BUFS];
    dictionary* ini;
    char* checksums;
    int i;
    int sectorID = rank / (VELOC_Mem_Topo->groupSize * VELOC_Mem_Topo->nodeSize);
    int node = rank / VELOC_Mem_Topo->nodeSize;
    int rankInGroup = node - (sectorID * VELOC_Mem_Topo->groupSize);
    int groupID = rank % VELOC_Mem_Topo->nodeSize;

    checksums = talloc(char, VELOC_Mem_Topo->groupSize * MD5_DIGEST_LENGTH);
    MPI_Allgather(checksum, MD5_DIGEST_LENGTH, MPI_CHAR, checksums, MD5_DIGEST_LENGTH, MPI_CHAR, VELOC_Mem_Exec->groupComm);
    if (rankInGroup) {
        free(checksums);
        return VELOC_SUCCESS;
    }

    sprintf(fileName, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, groupID);
    ini = iniparser_load(fileName);
    if (ini == NULL) {
        VELOC_Mem_Print("Temporary metadata file could NOT be parsed", VELOC_Mem_WARN);
        free(checksums);
        return VELOC_Mem_NSCS;
    }
    // Add metadata to dictionary
    for (i = 0; i < VELOC_Mem_Topo->groupSize; i++) {
        strncpy(buf, checksums + (i * MD5_DIGEST_LENGTH), MD5_DIGEST_LENGTH);
        sprintf(str, "%d:RSed_checksum", i);
        iniparser_set(ini, str, buf);
    }
    free(checksums);

    sprintf(str, "Recreating metadata file (%s)...", buf);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    FILE* fd = fopen(fileName, "w");
    if (fd == NULL) {
        VELOC_Mem_Print("Metadata file could NOT be opened.", VELOC_Mem_WARN);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    // Write metadata
    iniparser_dump_ini(ini, fd);

    if (fflush(fd) != 0) {
        VELOC_Mem_Print("Metadata file could NOT be flushed.", VELOC_Mem_WARN);

        iniparser_freedict(ini);
        fclose(fd);

        return VELOC_Mem_NSCS;
    }
    if (fclose(fd) != 0) {
        VELOC_Mem_Print("Metadata file could NOT be closed.", VELOC_Mem_WARN);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    iniparser_freedict(ini);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It gets the ptner file size from metadata.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      pfs             Pointer to fill the ptner file size.
    @param      group           The group in the node.
    @param      level           The level of the ckpt or 0 if tmp.
    @return     integer         VELOC_SUCCESS if successful.

    This function reads the metadata file created during checkpointing and
    reads partner file size.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_GetPtnerSize(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                      VELOCT_checkpoint* VELOC_Mem_Ckpt, unsigned long* pfs, int group, int level)
{
    dictionary* ini;
    char mfn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    if (level == 0) {
        sprintf(mfn, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, group);
    }
    else {
        sprintf(mfn, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Ckpt[level].metaDir, VELOC_Mem_Topo->sectorID, group);
    }
    sprintf(str, "Getting Ptner file size (%s)...", mfn);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    if (access(mfn, R_OK) != 0) {
        VELOC_Mem_Print("VELOC_Mem metadata file NOT accessible.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    ini = iniparser_load(mfn);
    if (ini == NULL) {
        VELOC_Mem_Print("Iniparser failed to parse the metadata file.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    //get Ptner file size
    sprintf(str, "%d:Ckpt_file_size", (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize);
    *pfs = iniparser_getlint(ini, str, -1);
    iniparser_freedict(ini);

    return VELOC_SUCCESS;
}


/*-------------------------------------------------------------------------*/
/**
    @brief      It gets the temporary metadata.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function reads the temporary metadata file created during checkpointing and
    recovers the checkpoint file name, file size, partner file size and the size
    of the largest file in the group (for padding if necessary during decoding).

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_LoadTmpMeta(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    if (!VELOC_Mem_Topo->amIaHead) {
        dictionary* ini;
        char metaFileName[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
        sprintf(metaFileName, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, VELOC_Mem_Topo->groupID);
        sprintf(str, "Getting VELOC_Mem metadata file (%s)...", metaFileName);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        if (access(metaFileName, R_OK) == 0) {
            ini = iniparser_load(metaFileName);
            if (ini == NULL) {
                VELOC_Mem_Print("Iniparser failed to parse the metadata file.", VELOC_Mem_WARN);
                return VELOC_Mem_NSCS;
            }
            else {
                VELOC_Mem_Exec->meta[0].exists[0] = 1;

                sprintf(str, "%d:Ckpt_file_name", VELOC_Mem_Topo->groupRank);
                char* ckptFileName = iniparser_getstring(ini, str, NULL);
                snprintf(VELOC_Mem_Exec->meta[0].ckptFile, VELOC_Mem_BUFS, "%s", ckptFileName);

                sprintf(str, "%d:Ckpt_file_size", VELOC_Mem_Topo->groupRank);
                VELOC_Mem_Exec->meta[0].fs[0] = iniparser_getlint(ini, str, -1);

                sprintf(str, "%d:Ckpt_file_size", (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize);
                VELOC_Mem_Exec->meta[0].pfs[0] = iniparser_getlint(ini, str, -1);

                VELOC_Mem_Exec->meta[0].maxFs[0] = iniparser_getlint(ini, "0:Ckpt_file_maxs", -1);

                iniparser_freedict(ini);
            }
        }
        else {
            VELOC_Mem_Print("Temporary metadata do not exist.", VELOC_Mem_WARN);
            return VELOC_Mem_NSCS;
        }
    }
    else { //I am a head
        int j, biggestCkptID = 0;
        for (j = 1; j < VELOC_Mem_Topo->nodeSize; j++) { //all body processes
            dictionary* ini;
            char metaFileName[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
            sprintf(metaFileName, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, j);
            sprintf(str, "Getting VELOC_Mem metadata file (%s)...", metaFileName);
            VELOC_Mem_Print(str, VELOC_Mem_DBUG);
            if (access(metaFileName, R_OK) == 0) {
                ini = iniparser_load(metaFileName);
                if (ini == NULL) {
                    VELOC_Mem_Print("Iniparser failed to parse the metadata file.", VELOC_Mem_WARN);
                    return VELOC_Mem_NSCS;
                }
                else {
                    VELOC_Mem_Exec->meta[0].exists[j] = 1;

                    sprintf(str, "%d:Ckpt_file_name", VELOC_Mem_Topo->groupRank);
                    char* ckptFileName = iniparser_getstring(ini, str, NULL);
                    snprintf(&VELOC_Mem_Exec->meta[0].ckptFile[j * VELOC_Mem_BUFS], VELOC_Mem_BUFS, "%s", ckptFileName);

                    //update head's ckptID
                    sscanf(&VELOC_Mem_Exec->meta[0].ckptFile[j * VELOC_Mem_BUFS], "Ckpt%d", &VELOC_Mem_Exec->ckptID);
                    if (VELOC_Mem_Exec->ckptID < biggestCkptID) {
                        VELOC_Mem_Exec->ckptID = biggestCkptID;
                    }

                    sprintf(str, "%d:Ckpt_file_size", VELOC_Mem_Topo->groupRank);
                    VELOC_Mem_Exec->meta[0].fs[j] = iniparser_getlint(ini, str, -1);

                    sprintf(str, "%d:Ckpt_file_size", (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize);
                    VELOC_Mem_Exec->meta[0].pfs[j] = iniparser_getlint(ini, str, -1);

                    VELOC_Mem_Exec->meta[0].maxFs[j] = iniparser_getlint(ini, "0:Ckpt_file_maxs", -1);

                    iniparser_freedict(ini);
                }
            }
            else {
                sprintf(str, "Temporary metadata do not exist for node process %d.", j);
                VELOC_Mem_Print(str, VELOC_Mem_WARN);
                return VELOC_Mem_NSCS;
            }
        }
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It gets the metadata to recover the data after a failure.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function reads the metadata file created during checkpointing and
    recovers the checkpoint file name, file size, partner file size and the size
    of the largest file in the group (for padding if necessary during decoding).

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_LoadMeta(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    int i, j;
    if (!VELOC_Mem_Topo->amIaHead) {
        //for each level
        for (i = 0; i < 5; i++) {
            dictionary* ini;
            char metaFileName[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
            if (i == 0) {
                sprintf(metaFileName, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, VELOC_Mem_Topo->groupID);
            } else {
                sprintf(metaFileName, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Ckpt[i].metaDir, VELOC_Mem_Topo->sectorID, VELOC_Mem_Topo->groupID);
            }
            sprintf(str, "Getting VELOC_Mem metadata file (%s)...", metaFileName);
            VELOC_Mem_Print(str, VELOC_Mem_DBUG);
            if (access(metaFileName, R_OK) == 0) {
                ini = iniparser_load(metaFileName);
                if (ini == NULL) {
                    VELOC_Mem_Print("Iniparser failed to parse the metadata file.", VELOC_Mem_WARN);
                    return VELOC_Mem_NSCS;
                }
                else {
                    sprintf(str, "Meta for level %d exists.", i);
                    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
                    VELOC_Mem_Exec->meta[i].exists[0] = 1;

                    sprintf(str, "%d:Ckpt_file_name", VELOC_Mem_Topo->groupRank);
                    char* ckptFileName = iniparser_getstring(ini, str, NULL);
                    snprintf(VELOC_Mem_Exec->meta[i].ckptFile, VELOC_Mem_BUFS, "%s", ckptFileName);

                    sprintf(str, "%d:Ckpt_file_size", VELOC_Mem_Topo->groupRank);
                    VELOC_Mem_Exec->meta[i].fs[0] = iniparser_getlint(ini, str, -1);

                    sprintf(str, "%d:Ckpt_file_size", (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize);
                    VELOC_Mem_Exec->meta[i].pfs[0] = iniparser_getlint(ini, str, -1);

                    VELOC_Mem_Exec->meta[i].maxFs[0] = iniparser_getlint(ini, "0:Ckpt_file_maxs", -1);

                    iniparser_freedict(ini);
                }
            }
        }
    }
    else { //I am a head
        int biggestCkptID = VELOC_Mem_Exec->ckptID;
        //for each level
        for (i = 0; i < 5; i++) {
            for (j = 1; j < VELOC_Mem_Topo->nodeSize; j++) { //for all body processes
                dictionary* ini;
                char metaFileName[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
                if (i == 0) {
                    sprintf(metaFileName, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, j);
                } else {
                    sprintf(metaFileName, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Ckpt[i].metaDir, VELOC_Mem_Topo->sectorID, j);
                }
                sprintf(str, "Getting VELOC_Mem metadata file (%s)...", metaFileName);
                VELOC_Mem_Print(str, VELOC_Mem_DBUG);
                if (access(metaFileName, R_OK) == 0) {
                    ini = iniparser_load(metaFileName);
                    if (ini == NULL) {
                        VELOC_Mem_Print("Iniparser failed to parse the metadata file.", VELOC_Mem_WARN);
                        return VELOC_Mem_NSCS;
                    }
                    else {
                        sprintf(str, "Meta for level %d exists.", i);
                        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
                        VELOC_Mem_Exec->meta[i].exists[j] = 1;

                        sprintf(str, "%d:Ckpt_file_name", VELOC_Mem_Topo->groupRank);
                        char* ckptFileName = iniparser_getstring(ini, str, NULL);
                        snprintf(&VELOC_Mem_Exec->meta[i].ckptFile[j * VELOC_Mem_BUFS], VELOC_Mem_BUFS, "%s", ckptFileName);

                        //update heads ckptID
                        sscanf(&VELOC_Mem_Exec->meta[i].ckptFile[j * VELOC_Mem_BUFS], "Ckpt%d", &VELOC_Mem_Exec->ckptID);
                        if (VELOC_Mem_Exec->ckptID < biggestCkptID) {
                            VELOC_Mem_Exec->ckptID = biggestCkptID;
                        }

                        sprintf(str, "%d:Ckpt_file_size", VELOC_Mem_Topo->groupRank);
                        VELOC_Mem_Exec->meta[i].fs[j] = iniparser_getlint(ini, str, -1);

                        sprintf(str, "%d:Ckpt_file_size", (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize);
                        VELOC_Mem_Exec->meta[i].pfs[j] = iniparser_getlint(ini, str, -1);

                        VELOC_Mem_Exec->meta[i].maxFs[j] = iniparser_getlint(ini, "0:Ckpt_file_maxs", -1);

                        iniparser_freedict(ini);
                    }
                }
            }
        }
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It writes the metadata to recover the data after a failure.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      fs              Pointer to the list of checkpoint sizes.
    @param      mfs             The maximum checkpoint file size.
    @param      fnl             Pointer to the list of checkpoint names.
    @param      checksums       Checksums array.
    @return     integer         VELOC_SUCCESS if successful.

    This function should be executed only by one process per group. It
    writes the metadata file used to recover in case of failure.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_WriteMetadata(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                      unsigned long* fs, unsigned long mfs, char* fnl, char* checksums)
{
    char str[VELOC_Mem_BUFS], buf[VELOC_Mem_BUFS];
    dictionary* ini;
    int i;

    snprintf(buf, VELOC_Mem_BUFS, "%s/Topology.velec_mem", VELOC_Mem_Conf->metadDir);
    sprintf(str, "Temporary load of topology file (%s)...", buf);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    // To bypass iniparser bug while empty dict.
    ini = iniparser_load(buf);
    if (ini == NULL) {
        VELOC_Mem_Print("Temporary topology file could NOT be parsed", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    // Add metadata to dictionary
    for (i = 0; i < VELOC_Mem_Topo->groupSize; i++) {
        strncpy(buf, fnl + (i * VELOC_Mem_BUFS), VELOC_Mem_BUFS - 1);
        sprintf(str, "%d", i);
        iniparser_set(ini, str, NULL);
        sprintf(str, "%d:Ckpt_file_name", i);
        iniparser_set(ini, str, buf);
        sprintf(str, "%d:Ckpt_file_size", i);
        sprintf(buf, "%lu", fs[i]);
        iniparser_set(ini, str, buf);
        sprintf(str, "%d:Ckpt_file_maxs", i);
        sprintf(buf, "%lu", mfs);
        iniparser_set(ini, str, buf);
        // TODO Checksums only local currently
        if (strlen(checksums)) {
            strncpy(buf, checksums + (i * MD5_DIGEST_LENGTH), MD5_DIGEST_LENGTH);
            sprintf(str, "%d:Ckpt_checksum", i);
            iniparser_set(ini, str, buf);
        }
    }

    // Remove topology section
    iniparser_unset(ini, "topology");
    if (mkdir(VELOC_Mem_Conf->mTmpDir, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("Cannot create directory", VELOC_Mem_EROR);
        }
    }

    sprintf(buf, "%s/sector%d-group%d.velec_mem", VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Topo->sectorID, VELOC_Mem_Topo->groupID);
    if (remove(buf) == -1) {
        if (errno != ENOENT) {
            VELOC_Mem_Print("Cannot remove sector-group.velec_mem", VELOC_Mem_EROR);
        }
    }

    sprintf(str, "Creating metadata file (%s)...", buf);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    FILE* fd = fopen(buf, "w");
    if (fd == NULL) {
        VELOC_Mem_Print("Metadata file could NOT be opened.", VELOC_Mem_WARN);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    // Write metadata
    iniparser_dump_ini(ini, fd);

    if (fclose(fd) != 0) {
        VELOC_Mem_Print("Metadata file could NOT be closed.", VELOC_Mem_WARN);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    iniparser_freedict(ini);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It writes the metadata to recover the data after a failure.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function gathers information about the checkpoint files in the
    group (name and sizes), and creates the metadata file used to recover in
    case of failure.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_CreateMetadata(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                       VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    char str[VELOC_Mem_BUFS], lfn[VELOC_Mem_BUFS], checksum[MD5_DIGEST_LENGTH];
    int i, res = VELOC_SUCCESS;
    int level = VELOC_Mem_Exec->ckptLvel;

    VELOC_Mem_Exec->meta[level].fs[0] = VELOC_Mem_Exec->ckptSize;
    long fs = VELOC_Mem_Exec->meta[level].fs[0]; // Gather all the file sizes
    long fileSizes[VELOC_Mem_BUFS];
    MPI_Allgather(&fs, 1, MPI_LONG, fileSizes, 1, MPI_LONG, VELOC_Mem_Exec->groupComm);

    //update partner file size:
    if (VELOC_Mem_Exec->ckptLvel == 2) {
        int ptnerGroupRank = (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize;
        VELOC_Mem_Exec->meta[2].pfs[0] = fileSizes[ptnerGroupRank];
    }

    long mfs = 0;
    for (i = 0; i < VELOC_Mem_Topo->groupSize; i++) {
        if (fileSizes[i] > mfs) {
            mfs = fileSizes[i]; // Search max. size
        }
    }
    VELOC_Mem_Exec->meta[VELOC_Mem_Exec->ckptLvel].maxFs[0] = mfs;
    sprintf(str, "Max. file size in group %lu.", mfs);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    char* ckptFileNames = talloc(char, VELOC_Mem_Topo->groupSize * VELOC_Mem_BUFS);
    strcpy(str, VELOC_Mem_Exec->meta[level].ckptFile); // Gather all the file names
    MPI_Gather(str, VELOC_Mem_BUFS, MPI_CHAR, ckptFileNames, VELOC_Mem_BUFS, MPI_CHAR, 0, VELOC_Mem_Exec->groupComm);

    // TODO Checksums only local currently
    char* checksums;
    if (!(level == 4 && VELOC_Mem_Ckpt[4].isInline)) {
        sprintf(lfn, "%s/%s", VELOC_Mem_Conf->lTmpDir, VELOC_Mem_Exec->meta[level].ckptFile);
        res = VELOC_Mem_Checksum(lfn, checksum);
        checksums = talloc(char, VELOC_Mem_Topo->groupSize * MD5_DIGEST_LENGTH);
        MPI_Allgather(checksum, MD5_DIGEST_LENGTH, MPI_CHAR, checksums, MD5_DIGEST_LENGTH, MPI_CHAR, VELOC_Mem_Exec->groupComm);
        if (res == VELOC_Mem_NSCS) {
            free(ckptFileNames);
            free(checksums);

            return VELOC_Mem_NSCS;
        }
    } else {
        //sprintf(lfn, "%s/%s", VELOC_Mem_Conf->gTmpDir, VELOC_Mem_Exec->meta[level].ckptFile);
        checksums = talloc(char, VELOC_Mem_BUFS);
        checksums[0]=0;
    }

    if (VELOC_Mem_Topo->groupRank == 0) { // Only one process in the group create the metadata
        res = VELOC_Mem_Try(VELOC_Mem_WriteMetadata(VELOC_Mem_Conf, VELOC_Mem_Topo, fileSizes, mfs, ckptFileNames, checksums), "write the metadata.");
        if (res == VELOC_Mem_NSCS) {
            free(ckptFileNames);
            free(checksums);

            return VELOC_Mem_NSCS;
        }
    }

    free(ckptFileNames);
    free(checksums);

    return VELOC_SUCCESS;
}
