/**
 *  @file   conf.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   December, 2013
 *  @brief  Configuration loading functions for the VELOC_Mem library.
 */

#include "interface.h"

/*-------------------------------------------------------------------------*/
/**
    @brief      Sets the exec. ID and failure parameters in the conf. file.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      restart         Value to set in the conf. file (0 or 1).
    @return     integer         VELOC_SUCCESS if successful.

    This function sets the execution ID and failure parameters in the
    configuration file. This is to avoid forcing the user to change these
    values manually in case of recovery needed. In this way, relaunching the
    execution in the same way as the initial time will make VELOC_Mem detect that
    it is a restart. It also allows to set the failure parameter back to 0
    at the end of a successful execution.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_UpdateConf(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec, int restart)
{
    char str[VELOC_Mem_BUFS];
    dictionary* ini;

    // Load dictionary
    ini = iniparser_load(VELOC_Mem_Conf->cfgFile);
    sprintf(str, "Updating configuration file (%s)...", VELOC_Mem_Conf->cfgFile);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    if (ini == NULL) {
        VELOC_Mem_Print("Iniparser failed to parse the conf. file.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    sprintf(str, "%d", restart);
    // Set failure to 'restart'
    iniparser_set(ini, "Restart:failure", str);
    // Set the exec. ID
    iniparser_set(ini, "Restart:exec_id", VELOC_Mem_Exec->id);

    FILE* fd = fopen(VELOC_Mem_Conf->cfgFile, "w");
    if (fd == NULL) {
        VELOC_Mem_Print("VELOC_Mem failed to open the configuration file.", VELOC_Mem_EROR);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    // Write new configuration
    iniparser_dump_ini(ini, fd);
    if (fflush(fd) != 0) {
        VELOC_Mem_Print("VELOC_Mem failed to flush the configuration file.", VELOC_Mem_EROR);

        iniparser_freedict(ini);
        fclose(fd);

        return VELOC_Mem_NSCS;
    }
    if (fclose(fd) != 0) {
        VELOC_Mem_Print("VELOC_Mem failed to close the configuration file.", VELOC_Mem_EROR);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    // Free dictionary
    iniparser_freedict(ini);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It reads the configuration given in the configuration file.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function reads the configuration given in the VELOC_Mem configuration
    file and sets other required parameters.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_ReadConf(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    // Check access to VELOC_Mem configuration file and load dictionary
    dictionary* ini;
    char *par, str[VELOC_Mem_BUFS];
    sprintf(str, "Reading VELOC_Mem configuration file (%s)...", VELOC_Mem_Conf->cfgFile);
    VELOC_Mem_Print(str, VELOC_Mem_INFO);
    if (access(VELOC_Mem_Conf->cfgFile, F_OK) != 0) {
        VELOC_Mem_Print("VELOC_Mem configuration file NOT accessible.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    ini = iniparser_load(VELOC_Mem_Conf->cfgFile);
    if (ini == NULL) {
        VELOC_Mem_Print("Iniparser failed to parse the conf. file.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    // Setting/reading checkpoint configuration metadata
    par = iniparser_getstring(ini, "Basic:ckpt_dir", NULL);
    snprintf(VELOC_Mem_Conf->localDir, VELOC_Mem_BUFS, "%s", par);
    par = iniparser_getstring(ini, "Basic:glbl_dir", NULL);
    snprintf(VELOC_Mem_Conf->glbalDir, VELOC_Mem_BUFS, "%s", par);
    par = iniparser_getstring(ini, "Basic:meta_dir", NULL);
    snprintf(VELOC_Mem_Conf->metadDir, VELOC_Mem_BUFS, "%s", par);
    VELOC_Mem_Ckpt[1].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l1", -1);
    VELOC_Mem_Ckpt[2].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l2", -1);
    VELOC_Mem_Ckpt[3].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l3", -1);
    VELOC_Mem_Ckpt[4].ckptIntv = (int)iniparser_getint(ini, "Basic:ckpt_l4", -1);
    VELOC_Mem_Ckpt[1].isInline = (int)1;
    VELOC_Mem_Ckpt[2].isInline = (int)iniparser_getint(ini, "Basic:inline_l2", 1);
    VELOC_Mem_Ckpt[3].isInline = (int)iniparser_getint(ini, "Basic:inline_l3", 1);
    VELOC_Mem_Ckpt[4].isInline = (int)iniparser_getint(ini, "Basic:inline_l4", 1);
    VELOC_Mem_Ckpt[1].ckptCnt  = 1;
    VELOC_Mem_Ckpt[2].ckptCnt  = 1;
    VELOC_Mem_Ckpt[3].ckptCnt  = 1;
    VELOC_Mem_Ckpt[4].ckptCnt  = 1;

    // Reading/setting configuration metadata
    VELOC_Mem_Conf->verbosity = (int)iniparser_getint(ini, "Basic:verbosity", -1);
    VELOC_Mem_Conf->saveLastCkpt = (int)iniparser_getint(ini, "Basic:keep_last_ckpt", 0);
    VELOC_Mem_Conf->blockSize = (int)iniparser_getint(ini, "Advanced:block_size", -1) * 1024;
    VELOC_Mem_Conf->transferSize = (int)iniparser_getint(ini, "Advanced:transfer_size", -1) * 1024 * 1024;
    VELOC_Mem_Conf->tag = (int)iniparser_getint(ini, "Advanced:mpi_tag", -1);
    VELOC_Mem_Conf->test = (int)iniparser_getint(ini, "Advanced:local_test", -1);
    VELOC_Mem_Conf->l3WordSize = VELOC_Mem_WORD;
    VELOC_Mem_Conf->ioMode = (int)iniparser_getint(ini, "Basic:ckpt_io", 0) + 1000;

    // Reading/setting execution metadata
    VELOC_Mem_Exec->nbVar = 0;
    VELOC_Mem_Exec->nbType = 0;
    VELOC_Mem_Exec->ckpt = 0;
    VELOC_Mem_Exec->minuteCnt = 0;
    VELOC_Mem_Exec->ckptCnt = 1;
    VELOC_Mem_Exec->ckptIcnt = 0;
    VELOC_Mem_Exec->ckptID = 0;
    VELOC_Mem_Exec->ckptLvel = 0;
    VELOC_Mem_Exec->ckptIntv = 1;
    VELOC_Mem_Exec->wasLastOffline = 0;
    VELOC_Mem_Exec->ckptNext = 0;
    VELOC_Mem_Exec->ckptLast = 0;
    VELOC_Mem_Exec->syncIter = 1;
    VELOC_Mem_Exec->lastIterTime = 0;
    VELOC_Mem_Exec->totalIterTime = 0;
    VELOC_Mem_Exec->meanIterTime = 0;
    VELOC_Mem_Exec->reco = (int)iniparser_getint(ini, "restart:failure", 0);
    if (VELOC_Mem_Exec->reco == 0) {
        time_t tim = time(NULL);
        struct tm* n = localtime(&tim);
        snprintf(VELOC_Mem_Exec->id, VELOC_Mem_BUFS, "%d-%02d-%02d_%02d-%02d-%02d",
            n->tm_year + 1900, n->tm_mon + 1, n->tm_mday, n->tm_hour, n->tm_min, n->tm_sec);
        MPI_Bcast(VELOC_Mem_Exec->id, VELOC_Mem_BUFS, MPI_CHAR, 0, VELOC_Mem_Exec->globalComm);
        sprintf(str, "The execution ID is: %s", VELOC_Mem_Exec->id);
        VELOC_Mem_Print(str, VELOC_Mem_INFO);
    }
    else {
        par = iniparser_getstring(ini, "restart:exec_id", NULL);
        snprintf(VELOC_Mem_Exec->id, VELOC_Mem_BUFS, "%s", par);
        sprintf(str, "This is a restart. The execution ID is: %s", VELOC_Mem_Exec->id);
        VELOC_Mem_Print(str, VELOC_Mem_INFO);
    }

    // Reading/setting topology metadata
    VELOC_Mem_Topo->nbHeads = (int)iniparser_getint(ini, "Basic:head", 0);
    VELOC_Mem_Topo->groupSize = (int)iniparser_getint(ini, "Basic:group_size", -1);
    VELOC_Mem_Topo->nodeSize = (int)iniparser_getint(ini, "Basic:node_size", -1);
    VELOC_Mem_Topo->nbApprocs = VELOC_Mem_Topo->nodeSize - VELOC_Mem_Topo->nbHeads;
    VELOC_Mem_Topo->nbNodes = (VELOC_Mem_Topo->nodeSize) ? VELOC_Mem_Topo->nbProc / VELOC_Mem_Topo->nodeSize : 0;

    // Synchronize after config reading and free dictionary
    MPI_Barrier(VELOC_Mem_Exec->globalComm);

    iniparser_freedict(ini);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It tests that the configuration given is correct.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tests the VELOC_Mem configuration to make sure that all
    parameter's values are correct.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_TestConfig(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                   VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    // Check if Reed-Salomon and L2 checkpointing is requested.
    int L2req = (VELOC_Mem_Ckpt[2].ckptIntv > 0) ? 1 : 0;
    int RSreq = (VELOC_Mem_Ckpt[3].ckptIntv > 0) ? 1 : 0;
    // Check requirements.
    if (VELOC_Mem_Topo->nbHeads != 0 && VELOC_Mem_Topo->nbHeads != 1) {
        VELOC_Mem_Print("The number of heads needs to be set to 0 or 1.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Topo->nbProc % VELOC_Mem_Topo->nodeSize != 0) {
        VELOC_Mem_Print("Number of ranks is not a multiple of the node size.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Topo->nbNodes % VELOC_Mem_Topo->groupSize != 0) {
        VELOC_Mem_Print("The number of nodes is not multiple of the group size.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Topo->groupSize <= 2 && (L2req || RSreq)) {
        VELOC_Mem_Print("The group size must be bigger than 2", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Topo->groupSize >= 32 && RSreq) {
        VELOC_Mem_Print("The group size must be lower than 32", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Conf->verbosity > 3 || VELOC_Mem_Conf->verbosity < 1) {
        VELOC_Mem_Print("Verbosity needs to be set to 1, 2 or 3.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Conf->blockSize > (2048 * 1024) || VELOC_Mem_Conf->blockSize < (1 * 1024)) {
        VELOC_Mem_Print("Block size needs to be set between 1 and 2048.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Conf->transferSize > (1024 * 1024 * 64) || VELOC_Mem_Conf->transferSize < (1024 * 1024 * 8)) {
        VELOC_Mem_Print("Transfer size (default = 16MB) not set in Cofiguration file.", VELOC_Mem_WARN);
        VELOC_Mem_Conf->transferSize = 16 * 1024 * 1024;
    }
    if (VELOC_Mem_Conf->test != 0 && VELOC_Mem_Conf->test != 1) {
        VELOC_Mem_Print("Local test size needs to be set to 0 or 1.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    if (VELOC_Mem_Conf->saveLastCkpt != 0 && VELOC_Mem_Conf->saveLastCkpt != 1) {
        VELOC_Mem_Print("Keep last ckpt. needs to be set to 0 or 1.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    int l;
    for (l = 1; l < 5; l++) {
        if (VELOC_Mem_Ckpt[l].ckptIntv == 0) {
            VELOC_Mem_Ckpt[l].ckptIntv = -1;
        }
        if (VELOC_Mem_Ckpt[l].isInline != 0 && VELOC_Mem_Ckpt[l].isInline != 1) {
            VELOC_Mem_Ckpt[l].isInline = 1;
        }
        if (VELOC_Mem_Ckpt[l].isInline == 0 && VELOC_Mem_Topo->nbHeads != 1) {
            VELOC_Mem_Print("If inline is set to 0 then head should be set to 1.", VELOC_Mem_WARN);
            return VELOC_Mem_NSCS;
        }
    }
    if (VELOC_Mem_Topo->groupSize < 1) {
        VELOC_Mem_Topo->groupSize = 1;
    }
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
    if (VELOC_Mem_Conf->ioMode < VELOC_Mem_IO_POSIX || VELOC_Mem_Conf->ioMode > VELOC_Mem_IO_SIONLIB) {
#else
    if (VELOC_Mem_Conf->ioMode < VELOC_Mem_IO_POSIX || VELOC_Mem_Conf->ioMode > VELOC_Mem_IO_MPI) {
#endif
        VELOC_Mem_Conf->ioMode = VELOC_Mem_IO_POSIX;
        VELOC_Mem_Print("No I/O selected. Set to default (POSIX)", VELOC_Mem_WARN);
    } else {
        switch(VELOC_Mem_Conf->ioMode) {
            case VELOC_Mem_IO_POSIX:
                VELOC_Mem_Print("Selected Ckpt I/O is POSIX", VELOC_Mem_INFO);
                break;
            case VELOC_Mem_IO_MPI:
                VELOC_Mem_Print("Selected Ckpt I/O is MPI-I/O", VELOC_Mem_INFO);
                break;
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
            case VELOC_Mem_IO_SIONLIB:
                VELOC_Mem_Print("Selected Ckpt I/O is SIONLIB", VELOC_Mem_INFO);
#endif
        }
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It tests that the directories given is correct.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tests that the directories given in the VELOC_Mem configuration
    are correct.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_TestDirectories(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo)
{
    char str[VELOC_Mem_BUFS];

    // Checking local directory
    sprintf(str, "Checking the local directory (%s)...", VELOC_Mem_Conf->localDir);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    if (mkdir(VELOC_Mem_Conf->localDir, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("The local directory could NOT be created.", VELOC_Mem_WARN);
            return VELOC_Mem_NSCS;
        }
    }

    if (VELOC_Mem_Topo->myRank == 0) {
        // Checking metadata directory
        sprintf(str, "Checking the metadata directory (%s)...", VELOC_Mem_Conf->metadDir);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        if (mkdir(VELOC_Mem_Conf->metadDir, 0777) == -1) {
            if (errno != EEXIST) {
                VELOC_Mem_Print("The metadata directory could NOT be created.", VELOC_Mem_WARN);
                return VELOC_Mem_NSCS;
            }
        }

        // Checking global directory
        sprintf(str, "Checking the global directory (%s)...", VELOC_Mem_Conf->glbalDir);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        if (mkdir(VELOC_Mem_Conf->glbalDir, 0777) == -1) {
            if (errno != EEXIST) {
                VELOC_Mem_Print("The global directory could NOT be created.", VELOC_Mem_WARN);
                return VELOC_Mem_NSCS;
            }
        }
    }
    //Waiting for metadDir being created
    MPI_Barrier(VELOC_Mem_COMM_WORLD);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It creates the directories required for current execution.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function creates the temporary metadata, local and global
    directories required for the current execution.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_CreateDirs(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                   VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    char fn[VELOC_Mem_BUFS];

    // Create metadata timestamp directory
    snprintf(fn, VELOC_Mem_BUFS, "%s/%s", VELOC_Mem_Conf->metadDir, VELOC_Mem_Exec->id);
    if (mkdir(fn, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("Cannot create metadata timestamp directory", VELOC_Mem_EROR);
        }
    }
    snprintf(VELOC_Mem_Conf->metadDir, VELOC_Mem_BUFS, "%s", fn);
    snprintf(VELOC_Mem_Conf->mTmpDir, VELOC_Mem_BUFS, "%s/tmp", fn);
    snprintf(VELOC_Mem_Ckpt[1].metaDir, VELOC_Mem_BUFS, "%s/l1", fn);
    snprintf(VELOC_Mem_Ckpt[2].metaDir, VELOC_Mem_BUFS, "%s/l2", fn);
    snprintf(VELOC_Mem_Ckpt[3].metaDir, VELOC_Mem_BUFS, "%s/l3", fn);
    snprintf(VELOC_Mem_Ckpt[4].metaDir, VELOC_Mem_BUFS, "%s/l4", fn);

    // Create global checkpoint timestamp directory
    snprintf(fn, VELOC_Mem_BUFS, "%s", VELOC_Mem_Conf->glbalDir);
    snprintf(VELOC_Mem_Conf->glbalDir, VELOC_Mem_BUFS, "%s/%s", fn, VELOC_Mem_Exec->id);
    if (mkdir(VELOC_Mem_Conf->glbalDir, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("Cannot create global checkpoint timestamp directory", VELOC_Mem_EROR);
        }
    }
    snprintf(VELOC_Mem_Conf->gTmpDir, VELOC_Mem_BUFS, "%s/tmp", VELOC_Mem_Conf->glbalDir);
    snprintf(VELOC_Mem_Ckpt[4].dir, VELOC_Mem_BUFS, "%s/l4", VELOC_Mem_Conf->glbalDir);

    // Create local checkpoint timestamp directory
    if (VELOC_Mem_Conf->test) { // If local test generate name by topology
        snprintf(fn, VELOC_Mem_BUFS, "%s/node%d", VELOC_Mem_Conf->localDir, VELOC_Mem_Topo->myRank / VELOC_Mem_Topo->nodeSize);
        if (mkdir(fn, 0777) == -1) {
            if (errno != EEXIST) {
                VELOC_Mem_Print("Cannot create local checkpoint timestamp directory", VELOC_Mem_EROR);
            }
        }
    }
    else {
        snprintf(fn, VELOC_Mem_BUFS, "%s", VELOC_Mem_Conf->localDir);
    }
    snprintf(VELOC_Mem_Conf->localDir, VELOC_Mem_BUFS, "%s/%s", fn, VELOC_Mem_Exec->id);
    if (mkdir(VELOC_Mem_Conf->localDir, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("Cannot create local checkpoint timestamp directory", VELOC_Mem_EROR);
        }
    }
    snprintf(VELOC_Mem_Conf->lTmpDir, VELOC_Mem_BUFS, "%s/tmp", VELOC_Mem_Conf->localDir);
    snprintf(VELOC_Mem_Ckpt[1].dir, VELOC_Mem_BUFS, "%s/l1", VELOC_Mem_Conf->localDir);
    snprintf(VELOC_Mem_Ckpt[2].dir, VELOC_Mem_BUFS, "%s/l2", VELOC_Mem_Conf->localDir);
    snprintf(VELOC_Mem_Ckpt[3].dir, VELOC_Mem_BUFS, "%s/l3", VELOC_Mem_Conf->localDir);
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It reads and tests the configuration given.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function reads the configuration file. Then test that the
    configuration parameters are correct (including directories).

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_LoadConf(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    int res;
    res = VELOC_Mem_Try(VELOC_Mem_ReadConf(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt), "read configuration.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Print("Impossible to read configuration.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    res = VELOC_Mem_Try(VELOC_Mem_TestConfig(VELOC_Mem_Conf, VELOC_Mem_Topo, VELOC_Mem_Ckpt), "pass the configuration test.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Print("Wrong configuration.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    res = VELOC_Mem_Try(VELOC_Mem_TestDirectories(VELOC_Mem_Conf, VELOC_Mem_Topo), "pass the directories test.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Print("Problem with the directories.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    res = VELOC_Mem_Try(VELOC_Mem_CreateDirs(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt), "create checkpoint directories.");
    if (res == VELOC_Mem_NSCS) {
        VELOC_Mem_Print("Problem creating the directories.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    return VELOC_SUCCESS;
}
