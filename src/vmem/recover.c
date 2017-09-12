/**
 *  @file   recover.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   January, 2014
 *  @brief  Recovery functions for the VELOC_Mem library.
 */

#include "interface.h"

/*-------------------------------------------------------------------------*/
/**
    @brief      It checks if a file exist and that its size is 'correct'.
    @param      fn              The ckpt. file name to check.
    @param      fs              The ckpt. file size to check.
    @param      checksum        The file checksum to check.
    @return     integer         0 if file exists, 1 if not or wrong size.

    This function checks whether a file exist or not and if its size is
    the expected one.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_CheckFile(char* fn, unsigned long fs, char* checksum)
{
    struct stat fileStatus;
    char str[VELOC_Mem_BUFS];
    if (access(fn, F_OK) == 0) {
        if (stat(fn, &fileStatus) == 0) {
            if (fileStatus.st_size == fs) {
                if (strlen(checksum)) {
                    int res = VELOC_Mem_VerifyChecksum(fn, checksum);
                    if (res != VELOC_SUCCESS) {
                        return 1;
                    }
                    return 0;
                }
                return 0;
            }
            else {
                return 1;
            }
        }
        else {
            return 1;
        }
    }
    else {
        sprintf(str, "Missing file: \"%s\"", fn);
        VELOC_Mem_Print(str, VELOC_Mem_WARN);
        return 1;
    }
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It detects all the erasures for a particular level.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      erased          The array of erasures to fill.
    @return     integer         VELOC_SUCCESS if successful.

    This function detects all the erasures for L1, L2 and L3. It return the
    results in the erased array. The search for erasures is done at the
    three levels independently on the current recovery level.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_CheckErasures(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                      VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                      int *erased)
{
    int buf;
    int ckptID, rank;
    int level = VELOC_Mem_Exec->ckptLvel;
    char fn[VELOC_Mem_BUFS];
    char checksum[MD5_DIGEST_LENGTH], ptnerChecksum[MD5_DIGEST_LENGTH], rsChecksum[MD5_DIGEST_LENGTH];
    long fs = VELOC_Mem_Exec->meta[level].fs[0];
    long pfs = VELOC_Mem_Exec->meta[level].pfs[0];
    long maxFs = VELOC_Mem_Exec->meta[level].maxFs[0];
    char ckptFile[VELOC_Mem_BUFS];
    strcpy(ckptFile, VELOC_Mem_Exec->meta[level].ckptFile);

    // TODO Checksums only local currently
    if ( level > 0 && level < 4 ) {
        VELOC_Mem_GetChecksums(VELOC_Mem_Conf, VELOC_Mem_Topo, VELOC_Mem_Ckpt, checksum, ptnerChecksum, rsChecksum, VELOC_Mem_Topo->groupID, level);
    }
    sprintf(fn, "Checking file %s and its erasures.", ckptFile);
    VELOC_Mem_Print(fn, VELOC_Mem_DBUG);
    switch (level) {
        case 1:
            sprintf(fn, "%s/%s", VELOC_Mem_Ckpt[1].dir, ckptFile);
            buf = VELOC_Mem_CheckFile(fn, fs, checksum);
            MPI_Allgather(&buf, 1, MPI_INT, erased, 1, MPI_INT, VELOC_Mem_Exec->groupComm);
            break;
        case 2:
            sprintf(fn, "%s/%s", VELOC_Mem_Ckpt[2].dir, ckptFile);
            buf = VELOC_Mem_CheckFile(fn, fs, checksum);
            MPI_Allgather(&buf, 1, MPI_INT, erased, 1, MPI_INT, VELOC_Mem_Exec->groupComm);
            sscanf(ckptFile, "Ckpt%d-Rank%d.velec_mem", &ckptID, &rank);
            sprintf(fn, "%s/Ckpt%d-Pcof%d.velec_mem", VELOC_Mem_Ckpt[2].dir, ckptID, rank);
            buf = VELOC_Mem_CheckFile(fn, pfs, ptnerChecksum);
            MPI_Allgather(&buf, 1, MPI_INT, erased + VELOC_Mem_Topo->groupSize, 1, MPI_INT, VELOC_Mem_Exec->groupComm);
            break;
        case 3:
            sprintf(fn, "%s/%s", VELOC_Mem_Ckpt[3].dir, ckptFile);
            buf = VELOC_Mem_CheckFile(fn, fs, checksum);
            MPI_Allgather(&buf, 1, MPI_INT, erased, 1, MPI_INT, VELOC_Mem_Exec->groupComm);
            sscanf(ckptFile, "Ckpt%d-Rank%d.velec_mem", &ckptID, &rank);
            sprintf(fn, "%s/Ckpt%d-RSed%d.velec_mem", VELOC_Mem_Ckpt[3].dir, ckptID, rank);
            buf = VELOC_Mem_CheckFile(fn, maxFs, rsChecksum);
            MPI_Allgather(&buf, 1, MPI_INT, erased + VELOC_Mem_Topo->groupSize, 1, MPI_INT, VELOC_Mem_Exec->groupComm);
            break;
        case 4:
            sprintf(fn, "%s/%s", VELOC_Mem_Ckpt[4].dir, ckptFile);
            buf = VELOC_Mem_CheckFile(fn, fs, "");
            MPI_Allgather(&buf, 1, MPI_INT, erased, 1, MPI_INT, VELOC_Mem_Exec->groupComm);
            break;
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It decides wich action take depending on the restart level.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function launches the required action depending on the recovery
    level. The recovery level is detected from the checkpoint ID of the
    last checkpoint taken.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_recoverFiles(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                     VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
   int r, tres = VELOC_SUCCESS, id, level = 1;
   unsigned long fs, maxFs;
   char str[VELOC_Mem_BUFS];
   VELOC_Mem_LoadMeta(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
   if (!VELOC_Mem_Topo->amIaHead) {
      while (level < 5) {
         if ((VELOC_Mem_Exec->reco == 2) && (level != 4)) {
            tres = VELOC_Mem_NSCS;
         }
         else {
               if (VELOC_Mem_Exec->meta[level].exists[0]) {
                   VELOC_Mem_Exec->ckptLvel = level;
                   switch (VELOC_Mem_Exec->ckptLvel) {
                      case 4:
                         VELOC_Mem_Clean(VELOC_Mem_Conf, VELOC_Mem_Topo, VELOC_Mem_Ckpt, 1, VELOC_Mem_Topo->groupID, VELOC_Mem_Topo->myRank);
                         MPI_Barrier(VELOC_Mem_COMM_WORLD);
                         sscanf(VELOC_Mem_Exec->meta[4].ckptFile, "Ckpt%d", &id);
                         sprintf(str, "Trying recovery with Ckpt. %d at level %d.", id, level);
                         VELOC_Mem_Print(str, VELOC_Mem_DBUG);
                         VELOC_Mem_Exec->ckptID = id;
                         VELOC_Mem_Exec->lastCkptLvel = VELOC_Mem_Exec->ckptLvel;
                         r = VELOC_Mem_recoverL4(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
                         break;
                      case 3:
                         sscanf(VELOC_Mem_Exec->meta[3].ckptFile, "Ckpt%d", &id);
                         sprintf(str, "Trying recovery with Ckpt. %d at level %d.", id, level);
                         VELOC_Mem_Print(str, VELOC_Mem_DBUG);
                         VELOC_Mem_Exec->ckptID = id;
                         VELOC_Mem_Exec->lastCkptLvel = VELOC_Mem_Exec->ckptLvel;
                         r = VELOC_Mem_recoverL3(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
                         break;
                      case 2:
                         sscanf(VELOC_Mem_Exec->meta[2].ckptFile, "Ckpt%d", &id);
                         sprintf(str, "Trying recovery with Ckpt. %d at level %d.", id, level);
                         VELOC_Mem_Print(str, VELOC_Mem_DBUG);
                         VELOC_Mem_Exec->ckptID = id;
                         VELOC_Mem_Exec->lastCkptLvel = VELOC_Mem_Exec->ckptLvel;
                         r = VELOC_Mem_recoverL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
                         break;
                      case 1:
                         sscanf(VELOC_Mem_Exec->meta[1].ckptFile, "Ckpt%d", &id);
                         sprintf(str, "Trying recovery with Ckpt. %d at level %d.", id, level);
                         VELOC_Mem_Print(str, VELOC_Mem_DBUG);
                         VELOC_Mem_Exec->ckptID = id;
                         VELOC_Mem_Exec->lastCkptLvel = VELOC_Mem_Exec->ckptLvel;
                         r = VELOC_Mem_recoverL1(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
                         break;
                   }
                   MPI_Allreduce(&r, &tres, 1, MPI_INT, MPI_SUM, VELOC_Mem_COMM_WORLD);
               }
               else {
                   tres = VELOC_Mem_NSCS;
               }
         }

         if (tres == VELOC_SUCCESS) {
            sprintf(str, "Recovering successfully from level %d.", level);
            VELOC_Mem_Print(str, VELOC_Mem_INFO);
            // This is to enable recovering from local for L4 case in VELOC_Mem_recover
            if (level == 4) {
                VELOC_Mem_Exec->ckptLvel = 1;
                if (VELOC_Mem_Topo->splitRank == 0) {
                    if (rename(VELOC_Mem_Ckpt[4].metaDir, VELOC_Mem_Ckpt[1].metaDir) == -1) {
                        VELOC_Mem_Print("Cannot rename L4 metadata folder to L1", VELOC_Mem_WARN);
                    }
                }
            }
            break;
         }
         else {
            sprintf(str, "No possible to restart from level %d.", level);
            VELOC_Mem_Print(str, VELOC_Mem_INFO);
            level++;
         }
      }
   }
   fs = tres;
   MPI_Allreduce(&fs, &tres, 1, MPI_INT, MPI_SUM, VELOC_Mem_Exec->globalComm);
   MPI_Barrier(VELOC_Mem_Exec->globalComm);
   sleep(1); // Global barrier and sleep for clearer output
   return tres;
}
