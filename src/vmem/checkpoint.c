/**
 *  @file   checkpoint.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   December, 2013
 *  @brief  Checkpointing functions for the VELOC_Mem library.
 */

#define _POSIX_C_SOURCE 200809L
#include <string.h>

#include "interface.h"

/*-------------------------------------------------------------------------*/
/**
    @brief      It updates the local and global mean iteration time.
    @param      VELOC_Mem_Exec        Execution metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function updates the local and global mean iteration time. It also
    recomputes the checkpoint interval in iterations and corrects the next
    checkpointing iteration based on the observed mean iteration duration.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_UpdateIterTime(VELOCT_execution* VELOC_Mem_Exec)
{
    int nbProcs, res;
    char str[VELOC_Mem_BUFS];
    double last = VELOC_Mem_Exec->iterTime;
    VELOC_Mem_Exec->iterTime = MPI_Wtime();
    if (VELOC_Mem_Exec->ckptIcnt > 0) {
        VELOC_Mem_Exec->lastIterTime = VELOC_Mem_Exec->iterTime - last;
        VELOC_Mem_Exec->totalIterTime = VELOC_Mem_Exec->totalIterTime + VELOC_Mem_Exec->lastIterTime;
        if (VELOC_Mem_Exec->ckptIcnt % VELOC_Mem_Exec->syncIter == 0) {
            VELOC_Mem_Exec->meanIterTime = VELOC_Mem_Exec->totalIterTime / VELOC_Mem_Exec->ckptIcnt;
            MPI_Allreduce(&VELOC_Mem_Exec->meanIterTime, &VELOC_Mem_Exec->globMeanIter, 1, MPI_DOUBLE, MPI_SUM, VELOC_Mem_COMM_WORLD);
            MPI_Comm_size(VELOC_Mem_COMM_WORLD, &nbProcs);
            VELOC_Mem_Exec->globMeanIter = VELOC_Mem_Exec->globMeanIter / nbProcs;
            if (VELOC_Mem_Exec->globMeanIter > 60) {
                VELOC_Mem_Exec->ckptIntv = 1;
            }
            else {
                VELOC_Mem_Exec->ckptIntv = rint(60.0 / VELOC_Mem_Exec->globMeanIter);
            }
            res = VELOC_Mem_Exec->ckptLast + VELOC_Mem_Exec->ckptIntv;
            if (VELOC_Mem_Exec->ckptLast == 0) {
                res = res + 1;
            }
            if (res >= VELOC_Mem_Exec->ckptIcnt) {
                VELOC_Mem_Exec->ckptNext = res;
            }
            sprintf(str, "Current iter : %d ckpt iter. : %d . Next ckpt. at iter. %d",
                    VELOC_Mem_Exec->ckptIcnt, VELOC_Mem_Exec->ckptIntv, VELOC_Mem_Exec->ckptNext);
            VELOC_Mem_print(str, VELOC_Mem_DBUG);
            if (VELOC_Mem_Exec->syncIter < (VELOC_Mem_Exec->ckptIntv / 2)) {
                VELOC_Mem_Exec->syncIter = VELOC_Mem_Exec->syncIter * 2;
                sprintf(str, "Iteration frequency : %.2f sec/iter => %d iter/min. Resync every %d iter.",
                    VELOC_Mem_Exec->globMeanIter, VELOC_Mem_Exec->ckptIntv, VELOC_Mem_Exec->syncIter);
                VELOC_Mem_print(str, VELOC_Mem_DBUG);
            }
        }
    }
    VELOC_Mem_Exec->ckptIcnt++; // Increment checkpoint loop counter
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It writes the checkpoint data in the target file.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      VELOC_Mem_Data        Dataset metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function checks whether the checkpoint needs to be local or remote,
    opens the target file and writes dataset per dataset, the checkpoint data,
    it finally flushes and closes the checkpoint file.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_WriteCkpt(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                  VELOCT_dataset* VELOC_Mem_Data)
{
    int res;
    char str[VELOC_Mem_BUFS];
    sprintf(str, "Starting writing checkpoint (ID: %d, Lvl: %d)",
                    VELOC_Mem_Exec->ckptID, VELOC_Mem_Exec->ckptLvel);
    VELOC_Mem_print(str, VELOC_Mem_DBUG);

    double tt = MPI_Wtime();

    //update ckpt file name
    snprintf(VELOC_Mem_Exec->meta[VELOC_Mem_Exec->ckptLvel].ckptFile, VELOC_Mem_BUFS,
                    "Ckpt%d-Rank%d.velec_mem", VELOC_Mem_Exec->ckptID, VELOC_Mem_Topo->myRank);

    if (VELOC_Mem_Ckpt[4].isInline && VELOC_Mem_Exec->ckptLvel == 4) {
        // create global temp directory
        if (mkdir(VELOC_Mem_Conf->gTmpDir, 0777) == -1) {
            if (errno != EEXIST) {
                VELOC_Mem_print("Cannot create global directory", VELOC_Mem_EROR);
                return VELOC_Mem_NSCS;
            }
        }

        switch (VELOC_Mem_Conf->ioMode) {
           case VELOC_Mem_IO_POSIX:
              res = VELOC_Mem_WritePosix(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, VELOC_Mem_Data);
              break;
           case VELOC_Mem_IO_MPI:
              res = VELOC_Mem_WriteMPI(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Data);
              break;
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
           case VELOC_Mem_IO_SIONLIB:
              res = VELOC_Mem_WriteSionlib(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Data);
              break;
#endif
        }
        if (res != VELOC_SUCCESS) {
            return VELOC_Mem_NSCS;
        }
    }
    else {
        // create local temp directory
        if (mkdir(VELOC_Mem_Conf->lTmpDir, 0777) == -1) {
            if (errno != EEXIST) {
                VELOC_Mem_print("Cannot create local directory", VELOC_Mem_EROR);
            }
        }
        res = VELOC_Mem_Try(VELOC_Mem_WritePosix(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, VELOC_Mem_Data),"write checkpoint to PFS");
    }

    sprintf(str, "Time writing checkpoint file : %f seconds.", MPI_Wtime() - tt);
    VELOC_Mem_print(str, VELOC_Mem_DBUG);

    res = VELOC_Mem_Try(VELOC_Mem_CreateMetadata(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt), "create metadata.");
    return res;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Decides wich action start depending on the ckpt. level.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      level           Cleaning checkpoint level.
    @param      group           Must be groupID if App-proc. or 1 if Head.
    @param      pr              Must be 1 if App-proc. or nbApprocs if Head.
    @return     integer         VELOC_SUCCESS if successful.

    This function cleans the checkpoints of a group or a single process.
    It does that for each group (application process in the node) if executed
    by the head, or only locally if executed by an application process. The
    parameter pr determines if the for loops have 1 or number of App. procs.
    iterations. The group parameter helps determine the groupID in both cases.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_GroupClean(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                   VELOCT_checkpoint* VELOC_Mem_Ckpt, int level, int group, int pr)
{
    int i, rank;
    if (level == 0) {
        VELOC_Mem_print("Error postprocessing checkpoint. Discarding checkpoint...", VELOC_Mem_WARN);
    }
    rank = VELOC_Mem_Topo->myRank;
    for (i = 0; i < pr; i++) {
        if (VELOC_Mem_Topo->amIaHead) {
            rank = VELOC_Mem_Topo->body[i];
        }
        VELOC_Mem_Clean(VELOC_Mem_Conf, VELOC_Mem_Topo, VELOC_Mem_Ckpt, level, i + group, rank);
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Decides wich action start depending on the ckpt. level.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      group           Must be groupID if App-proc. or 1 if Head.
    @param      fo              Must be -1 if App-proc. or 0 if Head.
    @param      pr              Must be 1 if App-proc. or nbApprocs if Head.
    @return     integer         VELOC_SUCCESS if successful.

    This function launches the required action dependeing on the ckpt. level.
    It does that for each group (application process in the node) if executed
    by the head, or only locally if executed by an application process. The
    parameter pr determines if the for loops have 1 or number of App. procs.
    iterations. The group parameter helps determine the groupID in both cases.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_PostCkpt(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                 int group, int fo, int pr)
{
    int i, tres, res, level, nodeFlag, globalFlag = !VELOC_Mem_Topo->splitRank;
    double t0, t1, t2, t3;
    char str[VELOC_Mem_BUFS];
    char catstr[VELOC_Mem_BUFS];

    t0 = MPI_Wtime();

    res = (VELOC_Mem_Exec->ckptLvel == (VELOC_Mem_REJW - VELOC_Mem_BASE)) ? VELOC_Mem_NSCS : VELOC_SUCCESS;
    MPI_Allreduce(&res, &tres, 1, MPI_INT, MPI_SUM, VELOC_Mem_COMM_WORLD);
    if (tres != VELOC_SUCCESS) {
        VELOC_Mem_GroupClean(VELOC_Mem_Conf, VELOC_Mem_Topo, VELOC_Mem_Ckpt, 0, group, pr);
        return VELOC_Mem_NSCS;
    }

    t1 = MPI_Wtime();

    switch (VELOC_Mem_Exec->ckptLvel) {
        case 4:
            res += VELOC_Mem_Flush(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, fo);
            break;
        case 3:
            res += VELOC_Mem_RSenc(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
            break;
        case 2:
            res += VELOC_Mem_Ptner(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
            break;
        case 1:
            res += VELOC_Mem_Local(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
            break;
    }

    MPI_Allreduce(&res, &tres, 1, MPI_INT, MPI_SUM, VELOC_Mem_COMM_WORLD);
    if (tres != VELOC_SUCCESS) {
        VELOC_Mem_GroupClean(VELOC_Mem_Conf, VELOC_Mem_Topo, VELOC_Mem_Ckpt, 0, group, pr);
        return VELOC_Mem_NSCS;
    }

    t2 = MPI_Wtime();

    VELOC_Mem_GroupClean(VELOC_Mem_Conf, VELOC_Mem_Topo, VELOC_Mem_Ckpt, VELOC_Mem_Exec->ckptLvel, group, pr);
    nodeFlag = (((!VELOC_Mem_Topo->amIaHead) && ((VELOC_Mem_Topo->nodeRank - VELOC_Mem_Topo->nbHeads) == 0)) || (VELOC_Mem_Topo->amIaHead)) ? 1 : 0;
    if (nodeFlag) {
        //Debug message needed to test nodeFlag (./tests/nodeFlag/nodeFlag.c)
        sprintf(str, "Has nodeFlag = 1 and nodeID = %d. CkptLvel = %d.", VELOC_Mem_Topo->nodeID, VELOC_Mem_Exec->ckptLvel);
        VELOC_Mem_print(str, VELOC_Mem_DBUG);
        if (!(VELOC_Mem_Ckpt[4].isInline && VELOC_Mem_Exec->ckptLvel == 4)) {
            level = (VELOC_Mem_Exec->ckptLvel != 4) ? VELOC_Mem_Exec->ckptLvel : 1;
            if (rename(VELOC_Mem_Conf->lTmpDir, VELOC_Mem_Ckpt[level].dir) == -1) {
                VELOC_Mem_print("Cannot rename local directory", VELOC_Mem_EROR);
            }
            else {
                VELOC_Mem_print("Local directory renamed", VELOC_Mem_DBUG);
            }
        }
    }
    if (globalFlag) {
        if (VELOC_Mem_Exec->ckptLvel == 4) {
            if (rename(VELOC_Mem_Conf->gTmpDir, VELOC_Mem_Ckpt[VELOC_Mem_Exec->ckptLvel].dir) == -1) {
                VELOC_Mem_print("Cannot rename global directory", VELOC_Mem_EROR);
            }
        }
        if (rename(VELOC_Mem_Conf->mTmpDir, VELOC_Mem_Ckpt[VELOC_Mem_Exec->ckptLvel].metaDir) == -1) {
            VELOC_Mem_print("Cannot rename meta directory", VELOC_Mem_EROR);
        }
    }
    MPI_Barrier(VELOC_Mem_COMM_WORLD);

    t3 = MPI_Wtime();

    sprintf(catstr, "Post-checkpoint took %.2f sec.", t3 - t0);
    sprintf(str, "%s (Ag:%.2fs, Pt:%.2fs, Cl:%.2fs)", catstr, t1 - t0, t2 - t1, t3 - t2);
    VELOC_Mem_print(str, VELOC_Mem_INFO);
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It listens for checkpoint notifications.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function listens for notifications from the application processes
    and takes the required actions after notification. This function is only
    executed by the head of the nodes and its complementary with the
    VELOC_Mem_Checkpoint function in terms of communications.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Listen(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
               VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    MPI_Status status;
    char str[VELOC_Mem_BUFS];
    int i, buf, res, flags[7];
    for (i = 0; i < 7; i++) { // Initialize flags
        flags[i] = 0;
    }
    VELOC_Mem_print("Head listening...", VELOC_Mem_DBUG);
    for (i = 0; i < VELOC_Mem_Topo->nbApprocs; i++) { // Iterate on the application processes in the node
        MPI_Recv(&buf, 1, MPI_INT, VELOC_Mem_Topo->body[i], VELOC_Mem_Conf->tag, VELOC_Mem_Exec->globalComm, &status);
        sprintf(str, "The head received a %d message", buf);
        VELOC_Mem_print(str, VELOC_Mem_DBUG);
        fflush(stdout);
        flags[buf - VELOC_Mem_BASE] = flags[buf - VELOC_Mem_BASE] + 1;
    }
    for (i = 1; i < 7; i++) {
        if (flags[i] == VELOC_Mem_Topo->nbApprocs) { // Determining checkpoint level
            VELOC_Mem_Exec->ckptLvel = i;
        }
    }
    if (flags[6] > 0) {
        VELOC_Mem_Exec->ckptLvel = 6;
    }
    if (VELOC_Mem_Exec->ckptLvel == 5) { // If we were asked to finalize
        return VELOC_Mem_ENDW;
    }
    res = VELOC_Mem_Try(VELOC_Mem_PostCkpt(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, 1, 0, VELOC_Mem_Topo->nbApprocs), "postprocess the checkpoint.");
    if (res == VELOC_SUCCESS) {
        VELOC_Mem_Exec->wasLastOffline = 1;
        VELOC_Mem_Exec->lastCkptLvel = VELOC_Mem_Exec->ckptLvel;
        res = VELOC_Mem_Exec->ckptLvel;
    }
    for (i = 0; i < VELOC_Mem_Topo->nbApprocs; i++) { // Send msg. to avoid checkpoint collision
        MPI_Send(&res, 1, MPI_INT, VELOC_Mem_Topo->body[i], VELOC_Mem_Conf->tag, VELOC_Mem_Exec->globalComm);
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Writes ckpt to PFS using POSIX.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      VELOC_Mem_Data        Dataset metadata.
    @return     integer         VELOC_SUCCESS if successful.

**/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_WritePosix(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                    VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                    VELOCT_dataset* VELOC_Mem_Data)
{
   VELOC_Mem_print("I/O mode: Posix.", VELOC_Mem_DBUG);
   char str[VELOC_Mem_BUFS], fn[VELOC_Mem_BUFS];
   int level = VELOC_Mem_Exec->ckptLvel;
   if (level == 4 && VELOC_Mem_Ckpt[4].isInline) {
       sprintf(fn, "%s/%s", VELOC_Mem_Conf->gTmpDir, VELOC_Mem_Exec->meta[4].ckptFile);
   }
   else {
       sprintf(fn, "%s/%s", VELOC_Mem_Conf->lTmpDir, VELOC_Mem_Exec->meta[level].ckptFile);
   }

   // open task local ckpt file
   FILE* fd = fopen(fn, "wb");
   if (fd == NULL) {
      sprintf(str, "VELOC_Mem checkpoint file (%s) could not be opened.", fn);
      VELOC_Mem_print(str, VELOC_Mem_EROR);

      return VELOC_Mem_NSCS;
   }

   // write data into ckpt file
   int i;
   for (i = 0; i < VELOC_Mem_Exec->nbVar; i++) {
      clearerr(fd);
      size_t written = 0;
      int fwrite_errno;
      while (written < VELOC_Mem_Data[i].count && !ferror(fd)) {
         errno = 0;
         written += fwrite(((char*)VELOC_Mem_Data[i].ptr) + (VELOC_Mem_Data[i].eleSize*written), VELOC_Mem_Data[i].eleSize, VELOC_Mem_Data[i].count - written, fd);
         fwrite_errno = errno;
      }
      if (ferror(fd)) {
         char error_msg[VELOC_Mem_BUFS];
         error_msg[0] = 0;
         strerror_r(fwrite_errno, error_msg, VELOC_Mem_BUFS);
         sprintf(str, "Dataset #%d could not be written: %s.", VELOC_Mem_Data[i].id, error_msg);
         VELOC_Mem_print(str, VELOC_Mem_EROR);
         fclose(fd);
         return VELOC_Mem_NSCS;
      }
   }

   // close file
   if (fclose(fd) != 0) {
      VELOC_Mem_print("VELOC_Mem checkpoint file could not be closed.", VELOC_Mem_EROR);

      return VELOC_Mem_NSCS;
   }

   return VELOC_SUCCESS;

}

/*-------------------------------------------------------------------------*/
/**
  @brief      Writes ckpt to PFS using MPI I/O.
  @param      VELOC_Mem_Conf        Configuration metadata.
  @param      VELOC_Mem_Exec        Execution metadata.
  @param      VELOC_Mem_Topo        Topology metadata.
  @param      VELOC_Mem_Data        Dataset metadata.
  @return     integer         VELOC_SUCCESS if successful.

	In here it is taken into account, that in MPIIO the count parameter
	in both, MPI_Type_contiguous and MPI_File_write_at, are integer
  types. The ckpt data is split into chunks of maximal (MAX_INT-1)/2
	elements to form contiguous data types. It was experienced, that
	if the size is greater then that, it may lead to problems.

**/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_WriteMPI(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
      VELOCT_topology* VELOC_Mem_Topo, VELOCT_dataset* VELOC_Mem_Data)
{
   VELOC_Mem_print("I/O mode: MPI-IO.", VELOC_Mem_DBUG);
   char str[VELOC_Mem_BUFS], mpi_err[VELOC_Mem_BUFS];

   // enable collective buffer optimization
   MPI_Info info;
   MPI_Info_create(&info);
   MPI_Info_set(info, "romio_cb_write", "enable");

   // TODO enable to set stripping unit in the config file (Maybe also other hints)
   // set stripping unit to 4MB
   MPI_Info_set(info, "stripping_unit", "4194304");

   MPI_Offset chunkSize = VELOC_Mem_Exec->ckptSize;

   VELOC_Mem_Exec->meta[4].fs[0] = chunkSize;

   // collect chunksizes of other ranks
   MPI_Offset* chunkSizes = talloc(MPI_Offset, VELOC_Mem_Topo->nbApprocs * VELOC_Mem_Topo->nbNodes);
   MPI_Allgather(&chunkSize, 1, MPI_OFFSET, chunkSizes, 1, MPI_OFFSET, VELOC_Mem_COMM_WORLD);

   char gfn[VELOC_Mem_BUFS], ckptFile[VELOC_Mem_BUFS];
   snprintf(ckptFile, VELOC_Mem_BUFS, "Ckpt%d-mpiio.velec_mem", VELOC_Mem_Exec->ckptID);
   sprintf(gfn, "%s/%s", VELOC_Mem_Conf->gTmpDir, ckptFile);
   // open parallel file (collective call)
   MPI_File pfh;
   int res = MPI_File_open(VELOC_Mem_COMM_WORLD, gfn, MPI_MODE_WRONLY|MPI_MODE_CREATE, info, &pfh);

   // check if successful
   if (res != 0) {
      errno = 0;
      MPI_Error_string(res, mpi_err, NULL);
      snprintf(str, VELOC_Mem_BUFS, "unable to create file [MPI ERROR - %i] %s", res, mpi_err);
      VELOC_Mem_print(str, VELOC_Mem_EROR);
      free(chunkSizes);
      return VELOC_Mem_NSCS;
   }

   // set file offset
   MPI_Offset offset = 0;
   int i;
   for (i = 0; i < VELOC_Mem_Topo->splitRank; i++) {
      offset += chunkSizes[i];
   }
   free(chunkSizes);

   for (i = 0; i < VELOC_Mem_Exec->nbVar; i++) {
       long pos = 0;
       long varSize = VELOC_Mem_Data[i].size;
       long bSize = VELOC_Mem_Conf->transferSize;
       while (pos < varSize) {
           if ((varSize - pos) < VELOC_Mem_Conf->transferSize) {
               bSize = varSize - pos;
           }

           MPI_Datatype dType;
           MPI_Type_contiguous(bSize, MPI_BYTE, &dType);
           MPI_Type_commit(&dType);

           res = MPI_File_write_at(pfh, offset, VELOC_Mem_Data[i].ptr, 1, dType, MPI_STATUS_IGNORE);
           // check if successful
           if (res != 0) {
               errno = 0;
               MPI_Error_string(res, mpi_err, NULL);
               snprintf(str, VELOC_Mem_BUFS, "Failed to write protected_var[%i] to PFS  [MPI ERROR - %i] %s", i, res, mpi_err);
               VELOC_Mem_print(str, VELOC_Mem_EROR);
               MPI_File_close(&pfh);
               return VELOC_Mem_NSCS;
           }
           MPI_Type_free(&dType);
           offset += bSize;
           pos = pos + bSize;
       }
   }
   MPI_File_close(&pfh);
   MPI_Info_free(&info);
   return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      Writes ckpt to PFS using SIONlib.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Data        Dataset metadata.
    @return     integer         VELOC_SUCCESS if successful.

**/
/*-------------------------------------------------------------------------*/
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
int VELOC_Mem_WriteSionlib(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
      VELOCT_topology* VELOC_Mem_Topo,VELOCT_dataset* VELOC_Mem_Data)
{
   int numFiles = 1;
   int nlocaltasks = 1;
   int* file_map = calloc(1, sizeof(int));
   int* ranks = talloc(int, 1);
   int* rank_map = talloc(int, 1);
   sion_int64* chunkSizes = talloc(sion_int64, 1);
   int fsblksize = -1;
   chunkSizes[0] = VELOC_Mem_Exec->ckptSize;
   ranks[0] = VELOC_Mem_Topo->splitRank;
   rank_map[0] = VELOC_Mem_Topo->splitRank;

   // open parallel file
   char fn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
   snprintf(str, VELOC_Mem_BUFS, "Ckpt%d-sionlib.velec_mem", VELOC_Mem_Exec->ckptID);
   sprintf(fn, "%s/%s", VELOC_Mem_Conf->gTmpDir, str);
   int sid = sion_paropen_mapped_mpi(fn, "wb,posix", &numFiles, VELOC_Mem_COMM_WORLD, &nlocaltasks, &ranks, &chunkSizes, &file_map, &rank_map, &fsblksize, NULL);

   // check if successful
   if (sid == -1) {
      errno = 0;
      VELOC_Mem_print("SIONlib: File could no be opened", VELOC_Mem_EROR);

      free(file_map);
      free(rank_map);
      free(ranks);
      free(chunkSizes);
      return VELOC_Mem_NSCS;
   }

   // set file pointer to corresponding block in sionlib file
   int res = sion_seek(sid, VELOC_Mem_Topo->splitRank, SION_CURRENT_BLK, SION_CURRENT_POS);

   // check if successful
   if (res != SION_SUCCESS) {
      errno = 0;
      VELOC_Mem_print("SIONlib: Could not set file pointer", VELOC_Mem_EROR);
      sion_parclose_mapped_mpi(sid);
      free(file_map);
      free(rank_map);
      free(ranks);
      free(chunkSizes);
      return VELOC_Mem_NSCS;
   }

   // write datasets into file
   int i;
   for (i = 0; i < VELOC_Mem_Exec->nbVar; i++) {
      // SIONlib write call
      res = sion_fwrite(VELOC_Mem_Data[i].ptr, VELOC_Mem_Data[i].size, 1, sid);

      // check if successful
      if (res < 0) {
         errno = 0;
         VELOC_Mem_print("SIONlib: Data could not be written", VELOC_Mem_EROR);
         res =  sion_parclose_mapped_mpi(sid);
         free(file_map);
         free(rank_map);
         free(ranks);
         free(chunkSizes);
         return VELOC_Mem_NSCS;
      }

   }

   // close parallel file
   if (sion_parclose_mapped_mpi(sid) == -1) {
       VELOC_Mem_print("Cannot close sionlib file.", VELOC_Mem_WARN);
       free(file_map);
       free(rank_map);
       free(ranks);
       free(chunkSizes);
      return VELOC_Mem_NSCS;
   }
   free(file_map);
   free(rank_map);
   free(ranks);
   free(chunkSizes);

   return VELOC_SUCCESS;
}
#endif
