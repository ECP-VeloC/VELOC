/**
 *  @file   postckpt.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   December, 2013
 *  @brief  Post-checkpointing functions for the VELOC_Mem library.
 */

#include "interface.h"

/*-------------------------------------------------------------------------*/
/**
  @brief      It returns VELOC_Mem_SCES.
  @param      VELOC_Mem_Conf        Configuration metadata.
  @param      VELOC_Mem_Exec        Execution metadata.
  @param      VELOC_Mem_Topo        Topology metadata.
  @param      VELOC_Mem_Ckpt        Checkpoint metadata.
  @return     integer         VELOC_Mem_SCES.

  This function just returns VELOC_Mem_SCES to have homogeneous code.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Local(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    VELOC_Mem_Print("Starting checkpoint post-processing L1", VELOC_Mem_DBUG);
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It sends Ckpt file.
  @param      VELOC_Mem_Conf        Configuration metadata.
  @param      VELOC_Mem_Exec        Execution metadata.
  @param      VELOC_Mem_Ckpt        Checkpoint metadata.
  @param      destination     destination group rank
  @param      postFlag        0 if postckpt done by approc, > 0 if by head
  @return     integer         VELOC_Mem_SCES if successful.

  This function sends ckpt file to partner process. Partner should call
  VELOC_Mem_RecvPtner to receive this file.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_SendCkpt(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                 int destination, int postFlag)
{
    int bytes; //bytes read by fread
    char lfn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    FILE* lfd;

    sprintf(lfn, "%s/%s", VELOC_Mem_Conf->lTmpDir, &VELOC_Mem_Exec->meta[0].ckptFile[postFlag * VELOC_Mem_BUFS]);

    if (postFlag) {
        sprintf(str, "L2 trying to access process's %d ckpt. file (%s).", postFlag, lfn);
    }
    else {
        sprintf(str, "L2 trying to access local ckpt. file (%s).", lfn);
    }
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    lfd = fopen(lfn, "rb");
    if (lfd == NULL) {
        VELOC_Mem_Print("VELOC_Mem failed to open L2 Ckpt. file.", VELOC_Mem_DBUG);
        return VELOC_Mem_NSCS;
    }

    char* buffer = talloc(char, VELOC_Mem_Conf->blockSize);
    long toSend = VELOC_Mem_Exec->meta[0].fs[postFlag]; //remaining data to send
    while (toSend > 0) {
        int sendSize = (toSend > VELOC_Mem_Conf->blockSize) ? VELOC_Mem_Conf->blockSize : toSend;
        bytes = fread(buffer, sizeof(char), sendSize, lfd);

        if (ferror(lfd)) {
            VELOC_Mem_Print("Error reading data from L2 ckpt file", VELOC_Mem_DBUG);

            free(buffer);
            fclose(lfd);

            return VELOC_Mem_NSCS;
        }

        MPI_Send(buffer, bytes, MPI_CHAR, destination, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->groupComm);
        toSend -= bytes;
    }

    free(buffer);
    fclose(lfd);

    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It receives Ptner file.
  @param      VELOC_Mem_Conf        Configuration metadata.
  @param      VELOC_Mem_Exec        Execution metadata.
  @param      VELOC_Mem_Ckpt        Checkpoint metadata.
  @param      source          souce group rank
  @param      postFlag        0 if postckpt done by approc, > 0 if by head
  @return     integer         VELOC_Mem_SCES if successful.

  This function receives ckpt file from partner process and saves it as
  Ptner file. Partner should call VELOC_Mem_SendCkpt to send file.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_RecvPtner(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec, VELOCT_checkpoint* VELOC_Mem_Ckpt,
                  int source, int postFlag)
{
    int ckptID, rank;
    char pfn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];

    //heads need to use ckptFile to get ckptID and rank
    sscanf(&VELOC_Mem_Exec->meta[0].ckptFile[postFlag * VELOC_Mem_BUFS], "Ckpt%d-Rank%d.velec_mem", &ckptID, &rank);
    sprintf(pfn, "%s/Ckpt%d-Pcof%d.velec_mem", VELOC_Mem_Conf->lTmpDir, ckptID, rank);
    sprintf(str, "L2 trying to access Ptner file (%s).", pfn);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    FILE* pfd = fopen(pfn, "wb");
    if (pfd == NULL) {
        VELOC_Mem_Print("VELOC_Mem failed to open L2 ptner file.", VELOC_Mem_DBUG);
        return VELOC_Mem_NSCS;
    }

    char* buffer = talloc(char, VELOC_Mem_Conf->blockSize);
    unsigned long toRecv = VELOC_Mem_Exec->meta[0].pfs[postFlag]; //remaining data to receive
    while (toRecv > 0) {
        int recvSize = (toRecv > VELOC_Mem_Conf->blockSize) ? VELOC_Mem_Conf->blockSize : toRecv;
        MPI_Recv(buffer, recvSize, MPI_CHAR, source, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->groupComm, MPI_STATUS_IGNORE);
        fwrite(buffer, sizeof(char), recvSize, pfd);

        if (ferror(pfd)) {
            VELOC_Mem_Print("Error writing data to L2 ptner file", VELOC_Mem_DBUG);

            free(buffer);
            fclose(pfd);

            return VELOC_Mem_NSCS;
        }
        toRecv -= recvSize;
    }

    free(buffer);
    fclose(pfd);

    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It copies ckpt. files in to the partner node.
  @param      VELOC_Mem_Conf        Configuration metadata.
  @param      VELOC_Mem_Exec        Execution metadata.
  @param      VELOC_Mem_Topo        Topology metadata.
  @param      VELOC_Mem_Ckpt        Checkpoint metadata.
  @return     integer         VELOC_Mem_SCES if successful.

  This function copies the checkpoint files into the partner node. It
  follows a ring, where the ring size is the group size given in the VELOC_Mem
  configuration file.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Ptner(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    int source = VELOC_Mem_Topo->left; //receive Ckpt file from this process
    int destination = VELOC_Mem_Topo->right; //send Ckpt file to this process
    int res;
    VELOC_Mem_Print("Starting checkpoint post-processing L2", VELOC_Mem_DBUG);
    VELOC_Mem_LoadTmpMeta(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
    int startProc, endProc;
    if (VELOC_Mem_Topo->amIaHead) { //post-processing for every process in the node
        startProc = 1;
        endProc = VELOC_Mem_Topo->nodeSize;
    }
    else { //post-processing only for itself
        startProc = 0;
        endProc = 1;
    }
    int i;
    for (i = startProc; i < endProc; i++) {
        if (VELOC_Mem_Topo->groupRank % 2) { //first send, then receive
            res = VELOC_Mem_SendCkpt(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, destination, i);
            if (res != VELOC_Mem_SCES) {
                return VELOC_Mem_NSCS;
            }
            res = VELOC_Mem_RecvPtner(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, source, i);
            if (res != VELOC_Mem_SCES) {
                return VELOC_Mem_NSCS;
            }
        } else { //first receive, then send
            res = VELOC_Mem_RecvPtner(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, source, i);
            if (res != VELOC_Mem_SCES) {
                return VELOC_Mem_NSCS;
            }
            res = VELOC_Mem_SendCkpt(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, destination, i);
            if (res != VELOC_Mem_SCES) {
                return VELOC_Mem_NSCS;
            }
        }
    }
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
  @brief      It performs RS encoding with the ckpt. files in to the group.
  @param      VELOC_Mem_Conf        Configuration metadata.
  @param      VELOC_Mem_Exec        Execution metadata.
  @param      VELOC_Mem_Topo        Topology metadata.
  @param      VELOC_Mem_Ckpt        Checkpoint metadata.
  @return     integer         VELOC_Mem_SCES if successful.

  This function performs the Reed-Solomon encoding for a given group. The
  checkpoint files are padded to the maximum size of the largest checkpoint
  file in the group +- the extra space to be a multiple of block size.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_RSenc(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
              VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    char *myData, *data, *coding, lfn[VELOC_Mem_BUFS], efn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    int *matrix, cnt, i, j, init, src, offset, dest, matVal, res, bs = VELOC_Mem_Conf->blockSize, rank;
    unsigned long maxFs, fs, ps, pos = 0;
    MPI_Request reqSend, reqRecv;
    MPI_Status status;
    int remBsize = bs;
    FILE *lfd, *efd;

    VELOC_Mem_Print("Starting checkpoint post-processing L3", VELOC_Mem_DBUG);
    res = VELOC_Mem_Try(VELOC_Mem_LoadTmpMeta(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt), "load temporary metadata.");
    if (res != VELOC_Mem_SCES) {
        return VELOC_Mem_NSCS;
    }

    int startProc, endProc;
    if (VELOC_Mem_Topo->amIaHead) {
        startProc = 1;
        endProc = VELOC_Mem_Topo->nodeSize;
    }
    else {
        startProc = 0;
        endProc = 1;
    }

    int proc;
    for (proc = startProc; proc < endProc; proc++) {
        long fs = VELOC_Mem_Exec->meta[0].fs[proc]; //ckpt file size
        long maxFs = VELOC_Mem_Exec->meta[0].maxFs[proc]; //max file size in group

        ps = ((maxFs / bs)) * bs;
        if (ps < maxFs) {
            ps = ps + bs;
        }

        sscanf(&VELOC_Mem_Exec->meta[0].ckptFile[proc * VELOC_Mem_BUFS], "Ckpt%d-Rank%d.velec_mem", &VELOC_Mem_Exec->ckptID, &rank);
        sprintf(lfn, "%s/%s", VELOC_Mem_Conf->lTmpDir, &VELOC_Mem_Exec->meta[0].ckptFile[proc * VELOC_Mem_BUFS]);
        sprintf(efn, "%s/Ckpt%d-RSed%d.velec_mem", VELOC_Mem_Conf->lTmpDir, VELOC_Mem_Exec->ckptID, rank);

        sprintf(str, "L3 trying to access local ckpt. file (%s).", lfn);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);

        //all files in group must have the same size
        if (truncate(lfn, maxFs) == -1) {
            VELOC_Mem_Print("Error with truncate on checkpoint file", VELOC_Mem_WARN);
            return VELOC_Mem_NSCS;
        }

        lfd = fopen(lfn, "rb");
        if (lfd == NULL) {
            VELOC_Mem_Print("VELOC_Mem failed to open L3 checkpoint file.", VELOC_Mem_EROR);
            return VELOC_Mem_NSCS;
        }

        efd = fopen(efn, "wb");
        if (efd == NULL) {
            VELOC_Mem_Print("VELOC_Mem failed to open encoded ckpt. file.", VELOC_Mem_EROR);

            fclose(lfd);

            return VELOC_Mem_NSCS;
        }

        myData = talloc(char, bs);
        coding = talloc(char, bs);
        data = talloc(char, 2 * bs);
        matrix = talloc(int, VELOC_Mem_Topo->groupSize* VELOC_Mem_Topo->groupSize);

        for (i = 0; i < VELOC_Mem_Topo->groupSize; i++) {
            for (j = 0; j < VELOC_Mem_Topo->groupSize; j++) {
                matrix[i * VELOC_Mem_Topo->groupSize + j] = galois_single_divide(1, i ^ (VELOC_Mem_Topo->groupSize + j), VELOC_Mem_Conf->l3WordSize);
            }
        }

        // For each block
        while (pos < ps) {
            if ((maxFs - pos) < bs) {
                remBsize = maxFs - pos;
            }

            // Reading checkpoint files
            size_t bytes = fread(myData, sizeof(char), remBsize, lfd);
            if (ferror(lfd)) {
                VELOC_Mem_Print("VELOC_Mem failed to read from L3 ckpt. file.", VELOC_Mem_EROR);

                free(data);
                free(matrix);
                free(coding);
                free(myData);
                fclose(lfd);
                fclose(efd);

                return VELOC_Mem_NSCS;
            }

            dest = VELOC_Mem_Topo->groupRank;
            i = VELOC_Mem_Topo->groupRank;
            offset = 0;
            init = 0;
            cnt = 0;

            // For each encoding
            while (cnt < VELOC_Mem_Topo->groupSize) {
                if (cnt == 0) {
                    memcpy(&(data[offset * bs]), myData, sizeof(char) * bytes);
                }
                else {
                    MPI_Wait(&reqSend, &status);
                    MPI_Wait(&reqRecv, &status);
                }

                // At every loop *but* the last one we send the data
                if (cnt != VELOC_Mem_Topo->groupSize - 1) {
                    dest = (dest + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize;
                    src = (i + 1) % VELOC_Mem_Topo->groupSize;
                    MPI_Isend(myData, bytes, MPI_CHAR, dest, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->groupComm, &reqSend);
                    MPI_Irecv(&(data[(1 - offset) * bs]), bs, MPI_CHAR, src, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->groupComm, &reqRecv);
                }

                matVal = matrix[VELOC_Mem_Topo->groupRank * VELOC_Mem_Topo->groupSize + i];
                // First copy or xor any data that does not need to be multiplied by a factor
                if (matVal == 1) {
                    if (init == 0) {
                        memcpy(coding, &(data[offset * bs]), bs);
                        init = 1;
                    }
                    else {
                        galois_region_xor(&(data[offset * bs]), coding, coding, bs);
                    }
                }

                // Then the data that needs to be multiplied by a factor
                if (matVal != 0 && matVal != 1) {
                    galois_w16_region_multiply(&(data[offset * bs]), matVal, bs, coding, init);
                    init = 1;
                }

                i = (i + 1) % VELOC_Mem_Topo->groupSize;
                offset = 1 - offset;
                cnt++;
            }

            // Writting encoded checkpoints
            fwrite(coding, sizeof(char), remBsize, efd);

            // Next block
            pos = pos + bs;
        }

        free(data);
        free(matrix);
        free(coding);
        free(myData);
        fclose(lfd);
        fclose(efd);

        if (truncate(lfn, fs) == -1) {
            VELOC_Mem_Print("Error with re-truncate on checkpoint file", VELOC_Mem_WARN);
            return VELOC_Mem_NSCS;
        }

        //write checksum in metadata
        char checksum[MD5_DIGEST_LENGTH];
        res = VELOC_Mem_Checksum(efn, checksum);
        if (res != VELOC_Mem_SCES) {
            return VELOC_Mem_NSCS;
        }
        res = VELOC_Mem_WriteRSedChecksum(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, rank, checksum);
    }

    return res;
}


/*-------------------------------------------------------------------------*/
/**
  @brief      It flushes the local ckpt. files in to the PFS.
  @param      VELOC_Mem_Conf        Configuration metadata.
  @param      VELOC_Mem_Exec        Execution metadata.
  @param      VELOC_Mem_Topo        Topology metadata.
  @param      VELOC_Mem_Ckpt        Checkpoint metadata.
  @param      level           The level from which ckpt. files are flushed.
  @return     integer         VELOC_Mem_SCES if successful.

  This function flushes the local checkpoint files in to the PFS.

 **/
 /*-------------------------------------------------------------------------*/
int VELOC_Mem_Flush(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level)
{
    if (level == -1) {
       return VELOC_Mem_SCES; // Fake call for inline PFS checkpoint
    }
    char str[VELOC_Mem_BUFS];
    sprintf(str, "Starting checkpoint post-processing L4 for level %d", level);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    // create global temp directory
    if (mkdir(VELOC_Mem_Conf->gTmpDir, 0777) == -1) {
       if (errno != EEXIST) {
          VELOC_Mem_Print("Cannot create global directory", VELOC_Mem_EROR);
          return VELOC_Mem_NSCS;
       }
    }
    int res = VELOC_Mem_Try(VELOC_Mem_LoadMeta(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt), "load metadata.");
    if (res != VELOC_Mem_SCES) {
        return VELOC_Mem_NSCS;
    }
    if (!VELOC_Mem_Ckpt[4].isInline || VELOC_Mem_Conf->ioMode == VELOC_Mem_IO_POSIX) {
        res = VELOC_Mem_FlushPosix(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, level);
    }
    else {
        switch(VELOC_Mem_Conf->ioMode) {
            case VELOC_Mem_IO_MPI:
                VELOC_Mem_FlushMPI(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, level);
                break;
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
            case VELOC_Mem_IO_SIONLIB:
                VELOC_Mem_FlushSionlib(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, level);
                break;
#endif
        }
    }
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It flushes the local ckpt. files in to the PFS using POSIX.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      level           The level from which ckpt. files are flushed.
    @return     integer         VELOC_Mem_SCES if successful.

    This function flushes the local checkpoint files in to the PFS.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_FlushPosix(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                    VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level)
{
    VELOC_Mem_Print("Starting checkpoint post-processing L4 using Posix IO.", VELOC_Mem_DBUG);
    int startProc, endProc, proc;
    if (VELOC_Mem_Topo->amIaHead) {
        startProc = 1;
        endProc = VELOC_Mem_Topo->nodeSize;
    }
    else {
        startProc = 0;
        endProc = 1;
    }

    for (proc = startProc; proc < endProc; proc++) {
        char str[VELOC_Mem_BUFS];
        sprintf(str, "Post-processing for proc %d started.", proc);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        char lfn[VELOC_Mem_BUFS], gfn[VELOC_Mem_BUFS];
        sprintf(gfn, "%s/%s", VELOC_Mem_Conf->gTmpDir, &VELOC_Mem_Exec->meta[level].ckptFile[proc * VELOC_Mem_BUFS]);
        sprintf(str, "Global temporary file name for proc %d: %s", proc, gfn);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        FILE* gfd = fopen(gfn, "wb");

        if (gfd == NULL) {
           VELOC_Mem_Print("L4 cannot open ckpt. file in the PFS.", VELOC_Mem_EROR);
           return VELOC_Mem_NSCS;
        }

        if (level == 0) {
            sprintf(lfn, "%s/%s", VELOC_Mem_Conf->lTmpDir, &VELOC_Mem_Exec->meta[0].ckptFile[proc * VELOC_Mem_BUFS]);
        }
        else {
            sprintf(lfn, "%s/%s", VELOC_Mem_Ckpt[level].dir, &VELOC_Mem_Exec->meta[level].ckptFile[proc * VELOC_Mem_BUFS]);
        }
        sprintf(str, "Local file name for proc %d: %s", proc, lfn);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        // Open local file
        FILE* lfd = fopen(lfn, "rb");
        if (lfd == NULL) {
            VELOC_Mem_Print("L4 cannot open the checkpoint file.", VELOC_Mem_EROR);
            fclose(gfd);
            return VELOC_Mem_NSCS;
        }

        char *readData = talloc(char, VELOC_Mem_Conf->transferSize);
        long bSize = VELOC_Mem_Conf->transferSize;
        long fs = VELOC_Mem_Exec->meta[level].fs[proc];
        sprintf(str, "Local file size for proc %d: %ld", proc, fs);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        long pos = 0;
        // Checkpoint files exchange
        while (pos < fs) {
            if ((fs - pos) < VELOC_Mem_Conf->transferSize)
              bSize = fs - pos;

            size_t bytes = fread(readData, sizeof(char), bSize, lfd);
            if (ferror(lfd)) {
                VELOC_Mem_Print("L4 cannot read from the ckpt. file.", VELOC_Mem_EROR);
                free(readData);
                fclose(lfd);
                fclose(gfd);
                return VELOC_Mem_NSCS;
           }

            fwrite(readData, sizeof(char), bytes, gfd);
            if (ferror(gfd)) {
                VELOC_Mem_Print("L4 cannot write to the ckpt. file in the PFS.", VELOC_Mem_EROR);
                free(readData);
                fclose(lfd);
                fclose(gfd);
                return VELOC_Mem_NSCS;
            }
            pos = pos + bytes;
        }
        free(readData);
        fclose(lfd);
        fclose(gfd);
    }
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It flushes the local ckpt. files in to the PFS using MPI-I/O.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      level           The level from which ckpt. files are flushed.
    @return     integer         VELOC_Mem_SCES if successful.

    This function flushes the local checkpoint files in to the PFS.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_FlushMPI(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                    VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level)
{
    VELOC_Mem_Print("Starting checkpoint post-processing L4 using MPI-IO.", VELOC_Mem_DBUG);
    // enable collective buffer optimization
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "romio_cb_write", "enable");
    // TODO enable to set stripping unit in the config file (Maybe also other hints)
    // set stripping unit to 4MB
    MPI_Info_set(info, "stripping_unit", "4194304");

    // open parallel file (collective call)
    MPI_File pfh; // MPI-IO file handle
    char gfn[VELOC_Mem_BUFS], lfn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    snprintf(str, VELOC_Mem_BUFS, "Ckpt%d-mpiio.velec_mem", VELOC_Mem_Exec->ckptID);
    sprintf(gfn, "%s/%s", VELOC_Mem_Conf->gTmpDir, str);
    int res = MPI_File_open(VELOC_Mem_COMM_WORLD, gfn, MPI_MODE_WRONLY|MPI_MODE_CREATE, info, &pfh);
    if (res != 0) {
       errno = 0;
       char mpi_err[VELOC_Mem_BUFS];
       int reslen;
       MPI_Error_string(res, mpi_err, &reslen);
       snprintf(str, VELOC_Mem_BUFS, "Unable to create file during MPI-IO flush [MPI ERROR - %i] %s", res, mpi_err);
       VELOC_Mem_Print(str, VELOC_Mem_EROR);
       MPI_Info_free(&info);
       return VELOC_Mem_NSCS;
    }
    MPI_Info_free(&info);

    int proc, startProc, endProc;
    if (VELOC_Mem_Topo->amIaHead) {
        startProc = 1;
        endProc = VELOC_Mem_Topo->nodeSize;
    }
    else {
        startProc = 0;
        endProc = 1;
    }
    int nbProc = endProc - startProc;
    MPI_Offset* localFileSizes = talloc(MPI_Offset, nbProc);
    char* localFileNames = talloc(char, VELOC_Mem_BUFS * endProc);
    int* splitRanks = talloc(int, endProc); //rank of process in VELOC_Mem_COMM_WORLD
    for (proc = startProc; proc < endProc; proc++) {
        if (level == 0) {
            sprintf(&localFileNames[proc * VELOC_Mem_BUFS], "%s/%s", VELOC_Mem_Conf->lTmpDir, &VELOC_Mem_Exec->meta[0].ckptFile[proc * VELOC_Mem_BUFS]);
        }
        else {
            sprintf(&localFileNames[proc * VELOC_Mem_BUFS], "%s/%s", VELOC_Mem_Ckpt[level].dir, &VELOC_Mem_Exec->meta[level].ckptFile[proc * VELOC_Mem_BUFS]);
        }
        if (VELOC_Mem_Topo->amIaHead) {
            splitRanks[proc] = (VELOC_Mem_Topo->nodeSize - 1) * VELOC_Mem_Topo->nodeID + proc - 1;
        }
        else {
            splitRanks[proc] = VELOC_Mem_Topo->splitRank;
        }
        localFileSizes[proc - startProc] = VELOC_Mem_Exec->meta[level].fs[proc]; //[proc - startProc] to get index from 0
    }

    MPI_Offset* allFileSizes = talloc(MPI_Offset, VELOC_Mem_Topo->nbApprocs * VELOC_Mem_Topo->nbNodes);
    MPI_Allgather(localFileSizes, nbProc, MPI_OFFSET, allFileSizes, nbProc, MPI_OFFSET, VELOC_Mem_COMM_WORLD);
    free(localFileSizes);

    for (proc = startProc; proc < endProc; proc++) {
        MPI_Offset offset = 0;
        int i;
        for (i = 0; i < splitRanks[proc]; i++) {
           offset += allFileSizes[i];
        }

        FILE* lfd = fopen(&localFileNames[VELOC_Mem_BUFS * proc], "rb");
        if (lfd == NULL) {
           VELOC_Mem_Print("L4 cannot open the checkpoint file.", VELOC_Mem_EROR);
           free(localFileNames);
           free(allFileSizes);
           free(splitRanks);
           return VELOC_Mem_NSCS;
        }

        char* readData = talloc(char, VELOC_Mem_Conf->transferSize);
        long bSize = VELOC_Mem_Conf->transferSize;
        long fs = VELOC_Mem_Exec->meta[level].fs[proc];

        long pos = 0;
        // Checkpoint files exchange
        while (pos < fs) {
            if ((fs - pos) < VELOC_Mem_Conf->transferSize) {
                bSize = fs - pos;
            }

            size_t bytes = fread(readData, sizeof(char), bSize, lfd);
            if (ferror(lfd)) {
              VELOC_Mem_Print("L4 cannot read from the ckpt. file.", VELOC_Mem_EROR);
              free(localFileNames);
              free(allFileSizes);
              free(splitRanks);
              free(readData);
              fclose(lfd);
              MPI_File_close(&pfh);
              return VELOC_Mem_NSCS;
            }
            MPI_Datatype dType;
            MPI_Type_contiguous(bytes, MPI_BYTE, &dType);
            MPI_Type_commit(&dType);

            res = MPI_File_write_at(pfh, offset, readData, 1, dType, MPI_STATUS_IGNORE);
            // check if successful
            if (res != 0) {
                errno = 0;
                int reslen;
                char mpi_err[VELOC_Mem_BUFS];
                MPI_Error_string(res, mpi_err, &reslen);
                snprintf(str, VELOC_Mem_BUFS, "Failed to write data to PFS during MPIIO Flush [MPI ERROR - %i] %s", res, mpi_err);
                VELOC_Mem_Print(str, VELOC_Mem_EROR);
                free(localFileNames);
                free(splitRanks);
                free(allFileSizes);
                fclose(lfd);
                MPI_File_close(&pfh);
                return VELOC_Mem_NSCS;
            }
            MPI_Type_free(&dType);
            offset += bytes;
            pos = pos + bytes;
        }
        free(readData);
        fclose(lfd);
    }
    free(localFileNames);
    free(allFileSizes);
    free(splitRanks);
    MPI_File_close(&pfh);
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It flushes the local ckpt. files in to the PFS using SIONlib.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      level           The level from which ckpt. files are flushed.
    @return     integer         VELOC_Mem_SCES if successful.

    This function flushes the local checkpoint files in to the PFS.

 **/
/*-------------------------------------------------------------------------*/
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
int VELOC_Mem_FlushSionlib(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
      VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int level)
{
    int proc, startProc, endProc;
    if (VELOC_Mem_Topo->amIaHead) {
        startProc = 1;
        endProc = VELOC_Mem_Topo->nodeSize;
    }
    else {
        startProc = 0;
        endProc = 1;
    }
    int nbProc = endProc - startProc;

    long* localFileSizes = talloc(long, nbProc);
    char* localFileNames = talloc(char, VELOC_Mem_BUFS * nbProc);
    int* splitRanks = talloc(int, nbProc); //rank of process in VELOC_Mem_COMM_WORLD
    for (proc = startProc; proc < endProc; proc++) {
        // Open local file case 0:
        if (level == 0) {
            sprintf(&localFileNames[proc * VELOC_Mem_BUFS], "%s/%s", VELOC_Mem_Conf->lTmpDir, &VELOC_Mem_Exec->meta[0].ckptFile[proc * VELOC_Mem_BUFS]);
        }
        else {
            sprintf(&localFileNames[proc * VELOC_Mem_BUFS], "%s/%s", VELOC_Mem_Ckpt[level].dir, &VELOC_Mem_Exec->meta[level].ckptFile[proc * VELOC_Mem_BUFS]);
        }
        if (VELOC_Mem_Topo->amIaHead) {
            splitRanks[proc - startProc] = (VELOC_Mem_Topo->nodeSize - 1) * VELOC_Mem_Topo->nodeID + proc - 1; //[proc - startProc] to get index from 0
        }
        else {
            splitRanks[proc - startProc] = VELOC_Mem_Topo->splitRank; //[proc - startProc] to get index from 0
        }
        localFileSizes[proc - startProc] = VELOC_Mem_Exec->meta[level].fs[proc]; //[proc - startProc] to get index from 0
    }

    int rank, ckptID;
    char fn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    sscanf(&VELOC_Mem_Exec->meta[level].ckptFile[0], "Ckpt%d-Rank%d.velec_mem", &ckptID, &rank);
    snprintf(str, VELOC_Mem_BUFS, "Ckpt%d-sionlib.velec_mem", ckptID);
    sprintf(fn, "%s/%s", VELOC_Mem_Conf->gTmpDir, str);

    int numFiles = 1;
    int nlocaltasks = nbProc;
    int* file_map = calloc(nbProc, sizeof(int));
    int* ranks = talloc(int, nbProc);
    int* rank_map = talloc(int, nbProc);
    sion_int64* chunkSizes = talloc(sion_int64, nbProc);
    int fsblksize = -1;
    int i;
    for (i = 0; i < nbProc; i++) {
        chunkSizes[i] = localFileSizes[i];
        ranks[i] = splitRanks[i];
        rank_map[i] = splitRanks[i];
    }
    int sid = sion_paropen_mapped_mpi(fn, "wb,posix", &numFiles, VELOC_Mem_COMM_WORLD, &nlocaltasks, &ranks, &chunkSizes, &file_map, &rank_map, &fsblksize, NULL);
    if (sid == -1) {
       VELOC_Mem_Print("Cannot open with sion_paropen_mapped_mpi.", VELOC_Mem_EROR);

       free(file_map);
       free(ranks);
       free(rank_map);
       free(chunkSizes);

       return VELOC_Mem_NSCS;
    }

    for (proc = startProc; proc < endProc; proc++) {
        FILE* lfd = fopen(&localFileNames[VELOC_Mem_BUFS * proc], "rb");
        if (lfd == NULL) {
           VELOC_Mem_Print("L4 cannot open the checkpoint file.", VELOC_Mem_EROR);
           free(localFileNames);
           free(splitRanks);
           sion_parclose_mapped_mpi(sid);
           free(file_map);
           free(ranks);
           free(rank_map);
           free(chunkSizes);
           return VELOC_Mem_NSCS;
        }


        int res = sion_seek(sid, splitRanks[proc - startProc], SION_CURRENT_BLK, SION_CURRENT_POS);
        if (res != SION_SUCCESS) {
            errno = 0;
            sprintf(str, "SIONlib: unable to set file pointer");
            VELOC_Mem_Print(str, VELOC_Mem_EROR);
            free(localFileNames);
            free(splitRanks);
            fclose(lfd);
            sion_parclose_mapped_mpi(sid);
            free(file_map);
            free(ranks);
            free(rank_map);
            free(chunkSizes);
            return VELOC_Mem_NSCS;
        }

        char *readData = talloc(char, VELOC_Mem_Conf->transferSize);
        long bSize = VELOC_Mem_Conf->transferSize;
        long fs = VELOC_Mem_Exec->meta[level].fs[proc];

        long pos = 0;
        // Checkpoint files exchange
        while (pos < fs) {
            if ((fs - pos) < VELOC_Mem_Conf->transferSize)
                bSize = fs - pos;

            size_t bytes = fread(readData, sizeof(char), bSize, lfd);
            if (ferror(lfd)) {
                VELOC_Mem_Print("L4 cannot read from the ckpt. file.", VELOC_Mem_EROR);
                free(localFileNames);
                free(splitRanks);
                free(readData);
                fclose(lfd);
                sion_parclose_mapped_mpi(sid);
                free(file_map);
                free(ranks);
                free(rank_map);
                free(chunkSizes);
                return VELOC_Mem_NSCS;
            }

            long data_written = sion_fwrite(readData, sizeof(char), bytes, sid);

            if (data_written < 0) {
                VELOC_Mem_Print("Sionlib: could not write data", VELOC_Mem_EROR);
                free(localFileNames);
                free(splitRanks);
                free(readData);
                fclose(lfd);
                sion_parclose_mapped_mpi(sid);
                free(file_map);
                free(ranks);
                free(rank_map);
                free(chunkSizes);
                return VELOC_Mem_NSCS;
            }

            pos = pos + bytes;
        }
    }
    free(localFileNames);
    free(splitRanks);
    sion_parclose_mapped_mpi(sid);
    free(file_map);
    free(ranks);
    free(rank_map);
    free(chunkSizes);
}
#endif
