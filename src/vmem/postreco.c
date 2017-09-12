/**
 *  @file   postreco.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   January, 2014
 *  @brief  Post recovery functions for the VELOC_Mem library.
 */

#include "interface.h"

/*-------------------------------------------------------------------------*/
/**
    @brief      It recovers a set of ckpt. files using RS decoding.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      erased          The array of erasures.
    @return     integer         VELOC_SUCCESS if successful.

    This function tries to recover the L3 ckpt. files missing using the
    RS decoding.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Decode(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
               VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt, int* erased)
{
    int *matrix, *decMatrix, *dm_ids, *tmpmat, i, j, k, m, ps, bs, pos = 0;
    char **coding, **data, *dataTmp, fn[VELOC_Mem_BUFS], efn[VELOC_Mem_BUFS];
    FILE *fd, *efd;
    long fs = VELOC_Mem_Exec->meta[3].fs[0];
    long maxFs = VELOC_Mem_Exec->meta[3].maxFs[0];

    bs = VELOC_Mem_Conf->blockSize;
    k = VELOC_Mem_Topo->groupSize;
    m = k;
    ps = ((maxFs / VELOC_Mem_Conf->blockSize)) * VELOC_Mem_Conf->blockSize;
    if (ps < maxFs) {
        ps = ps + VELOC_Mem_Conf->blockSize; // Calculating padding size
    }

    if (mkdir(VELOC_Mem_Ckpt[3].dir, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("Cannot create directory", VELOC_Mem_EROR);
        }
    }

    int ckptID, rank;
    sscanf(VELOC_Mem_Exec->meta[3].ckptFile, "Ckpt%d-Rank%d.velec_mem", &ckptID, &rank);
    sprintf(efn, "%s/Ckpt%d-RSed%d.velec_mem", VELOC_Mem_Ckpt[3].dir, ckptID, rank);
    sprintf(fn, "%s/%s", VELOC_Mem_Ckpt[3].dir, VELOC_Mem_Exec->meta[3].ckptFile);

    data = talloc(char*, k);
    coding = talloc(char*, m);
    dataTmp = talloc(char, VELOC_Mem_Conf->blockSize* k);
    dm_ids = talloc(int, k);
    decMatrix = talloc(int, k* k);
    tmpmat = talloc(int, k* k);
    matrix = talloc(int, k* k);
    for (i = 0; i < VELOC_Mem_Topo->groupSize; i++) {
        for (j = 0; j < VELOC_Mem_Topo->groupSize; j++) {
            matrix[i * VELOC_Mem_Topo->groupSize + j] = galois_single_divide(1, i ^ (VELOC_Mem_Topo->groupSize + j), VELOC_Mem_Conf->l3WordSize);
        }
    }
    for (i = 0; i < m; i++) {
        coding[i] = talloc(char, VELOC_Mem_Conf->blockSize);
        data[i] = talloc(char, VELOC_Mem_Conf->blockSize);
    }
    j = 0;
    for (i = 0; j < k; i++) {
        if (erased[i] == 0) {
            dm_ids[j] = i;
            j++;
        }
    }
    // Building the matrix
    for (i = 0; i < k; i++) {
        if (dm_ids[i] < k) {
            for (j = 0; j < k; j++) {
                tmpmat[i * k + j] = 0;
            }
            tmpmat[i * k + dm_ids[i]] = 1;
        }
        else {
            for (j = 0; j < k; j++) {
                tmpmat[i * k + j] = matrix[(dm_ids[i] - k) * k + j];
            }
        }
    }
    // Inversing the matrix
    if (jerasure_invert_matrix(tmpmat, decMatrix, k, VELOC_Mem_Conf->l3WordSize) < 0) {
        VELOC_Mem_Print("Error inversing matrix", VELOC_Mem_DBUG);

        for (i = 0; i < m; i++) {
            free(coding[i]);
            free(data[i]);
        }
        free(tmpmat);
        free(dm_ids);
        free(decMatrix);
        free(matrix);
        free(data);
        free(dataTmp);
        free(coding);

        return VELOC_Mem_NSCS;
    }
    if (erased[VELOC_Mem_Topo->groupRank] == 0) { // Resize and open files
        if (truncate(fn, ps) == -1) {
            VELOC_Mem_Print("Error with truncate on checkpoint file", VELOC_Mem_DBUG);

            for (i = 0; i < m; i++) {
                free(coding[i]);
                free(data[i]);
            }

            free(tmpmat);
            free(dm_ids);
            free(decMatrix);
            free(matrix);
            free(data);
            free(dataTmp);
            free(coding);

            return VELOC_Mem_NSCS;
        }
        fd = fopen(fn, "rb");
    }
    else {
        fd = fopen(fn, "wb");
    }

    if (erased[VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize] == 0) {
        efd = fopen(efn, "rb");
    }
    else {
        efd = fopen(efn, "wb");
    }
    if (fd == NULL) {
        VELOC_Mem_Print("R3 cannot open checkpoint file.", VELOC_Mem_DBUG);

        if (efd) {
            fclose(efd);
        }
        for (i = 0; i < m; i++) {
            free(coding[i]);
            free(data[i]);
        }
        free(tmpmat);
        free(dm_ids);
        free(decMatrix);
        free(matrix);
        free(data);
        free(dataTmp);
        free(coding);

        return VELOC_Mem_NSCS;
    }
    if (efd == NULL) {
        VELOC_Mem_Print("R3 cannot open encoded ckpt. file.", VELOC_Mem_DBUG);

        fclose(fd);

        for (i = 0; i < m; i++) {
            free(coding[i]);
            free(data[i]);
        }
        free(tmpmat);
        free(dm_ids);
        free(decMatrix);
        free(matrix);
        free(data);
        free(dataTmp);
        free(coding);

        return VELOC_Mem_NSCS;
    }

    // Main loop, block by block
    while (pos < ps) {
        // Reading the data
        if (erased[VELOC_Mem_Topo->groupRank] == 0) {
            fread(data[VELOC_Mem_Topo->groupRank] + 0, sizeof(char), bs, fd);

            if (ferror(fd)) {
                VELOC_Mem_Print("R3 cannot from the ckpt. file.", VELOC_Mem_DBUG);

                fclose(fd);
                fclose(efd);

                for (i = 0; i < m; i++) {
                    free(coding[i]);
                    free(data[i]);
                }
                free(tmpmat);
                free(dm_ids);
                free(decMatrix);
                free(matrix);
                free(data);
                free(dataTmp);
                free(coding);

                return VELOC_Mem_NSCS;
            }
        }
        else {
            bzero(data[VELOC_Mem_Topo->groupRank], bs);
        } // Erasure found

        if (erased[VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize] == 0) {
            fread(coding[VELOC_Mem_Topo->groupRank] + 0, sizeof(char), bs, efd);
            if (ferror(efd)) {
                VELOC_Mem_Print("R3 cannot from the encoded ckpt. file.", VELOC_Mem_DBUG);

                fclose(fd);
                fclose(efd);

                for (i = 0; i < m; i++) {
                    free(coding[i]);
                    free(data[i]);
                }
                free(tmpmat);
                free(dm_ids);
                free(decMatrix);
                free(matrix);
                free(data);
                free(dataTmp);
                free(coding);

                return VELOC_Mem_NSCS;
            }
        }
        else {
            bzero(coding[VELOC_Mem_Topo->groupRank], bs);
        }

        MPI_Allgather(data[VELOC_Mem_Topo->groupRank] + 0, bs, MPI_CHAR, dataTmp, bs, MPI_CHAR, VELOC_Mem_Exec->groupComm);
        for (i = 0; i < k; i++) {
            memcpy(data[i] + 0, &(dataTmp[i * bs]), sizeof(char) * bs);
        }

        MPI_Allgather(coding[VELOC_Mem_Topo->groupRank] + 0, bs, MPI_CHAR, dataTmp, bs, MPI_CHAR, VELOC_Mem_Exec->groupComm);
        for (i = 0; i < k; i++) {
            memcpy(coding[i] + 0, &(dataTmp[i * bs]), sizeof(char) * bs);
        }

        // Decoding the lost data work
        if (erased[VELOC_Mem_Topo->groupRank]) {
            jerasure_matrix_dotprod(k, VELOC_Mem_Conf->l3WordSize, decMatrix + (VELOC_Mem_Topo->groupRank * k), dm_ids, VELOC_Mem_Topo->groupRank, data, coding, bs);
        }

        MPI_Allgather(data[VELOC_Mem_Topo->groupRank] + 0, bs, MPI_CHAR, dataTmp, bs, MPI_CHAR, VELOC_Mem_Exec->groupComm);
        for (i = 0; i < k; i++) {
            memcpy(data[i] + 0, &(dataTmp[i * bs]), sizeof(char) * bs);
        }

        // Finally, re-encode any erased encoded checkpoint file
        if (erased[VELOC_Mem_Topo->groupRank + k]) {
            jerasure_matrix_dotprod(k, VELOC_Mem_Conf->l3WordSize, matrix + (VELOC_Mem_Topo->groupRank * k), NULL, VELOC_Mem_Topo->groupRank + k, data, coding, bs);
        }
        if (erased[VELOC_Mem_Topo->groupRank]) {
            fwrite(data[VELOC_Mem_Topo->groupRank] + 0, sizeof(char), bs, fd);
        }
        if (erased[VELOC_Mem_Topo->groupRank + k]) {
            fwrite(coding[VELOC_Mem_Topo->groupRank] + 0, sizeof(char), bs, efd);
        }

        pos = pos + bs;
    }

    // Closing files
    fclose(fd);
    fclose(efd);

    if (truncate(fn, fs) == -1) {
        VELOC_Mem_Print("R3 cannot re-truncate checkpoint file.", VELOC_Mem_WARN);

        for (i = 0; i < m; i++) {
            free(coding[i]);
            free(data[i]);
        }
        free(tmpmat);
        free(dm_ids);
        free(decMatrix);
        free(matrix);
        free(data);
        free(dataTmp);
        free(coding);

        return VELOC_Mem_NSCS;
    }

    for (i = 0; i < m; i++) {
        free(coding[i]);
        free(data[i]);
    }
    free(tmpmat);
    free(dm_ids);
    free(decMatrix);
    free(matrix);
    free(data);
    free(dataTmp);
    free(coding);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It checks that all L1 ckpt. files are present.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function detects all the erasures for L1. If there is at least one,
    L1 is not considered as recoverable.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_recoverL1(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    int erased[VELOC_Mem_BUFS], buf, j; // VELOC_Mem_BUFS > 32*3
    unsigned long fs, maxFs;
    if (VELOC_Mem_CheckErasures(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, erased) != VELOC_SUCCESS) {
        VELOC_Mem_Print("Error checking erasures.", VELOC_Mem_DBUG);
        return VELOC_Mem_NSCS;
    }
    buf = 0;
    for (j = 0; j < VELOC_Mem_Topo->groupSize; j++) {
        if (erased[j]) {
            buf++; // Counting erasures
        }
    }
    if (buf > 0) {
        VELOC_Mem_Print("Checkpoint files missing at L1.", VELOC_Mem_DBUG);
        return VELOC_Mem_NSCS;
    }
    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It sends checkpint file.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      destination     destination group rank.
    @param      ptner           0 if sending Ckpt, 1 if PtnerCkpt.
    @return     integer         VELOC_SUCCESS if successful.

    This function sends Ckpt or PtnerCkpt file from partner proccess.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_SendCkptFileL2(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                        VELOCT_checkpoint* VELOC_Mem_Ckpt, int destination, int ptner) {
    char filename[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    FILE *fileDesc;

    long toSend ; // remaining data to send
    if (ptner) {    //if want to send Ptner file
        int ckptID, rank;
        sscanf(VELOC_Mem_Exec->meta[2].ckptFile, "Ckpt%d-Rank%d.velec_mem", &ckptID, &rank); //do we need this from filename?
        sprintf(filename, "%s/Ckpt%d-Pcof%d.velec_mem", VELOC_Mem_Ckpt[2].dir, ckptID, rank);
        toSend = VELOC_Mem_Exec->meta[2].pfs[0];
    } else {    //if want to send Ckpt file
        sprintf(filename, "%s/%s", VELOC_Mem_Ckpt[2].dir, VELOC_Mem_Exec->meta[2].ckptFile);
        toSend = VELOC_Mem_Exec->meta[2].fs[0];
    }

    sprintf(str, "Opening file (rb) (%s) (L2).", filename);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    fileDesc = fopen(filename, "rb");
    if (fileDesc == NULL) {
        VELOC_Mem_Print("R2 cannot open the partner ckpt. file.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    char* buffer = talloc(char, VELOC_Mem_Conf->blockSize);

    while (toSend > 0) {
        int sendSize = (toSend > VELOC_Mem_Conf->blockSize) ? VELOC_Mem_Conf->blockSize : toSend;
        size_t bytes = fread(buffer, sizeof(char), sendSize, fileDesc);

        if (ferror(fileDesc)) {
            VELOC_Mem_Print("Error reading the data from the ckpt. file.", VELOC_Mem_WARN);

            fclose(fileDesc);
            free(buffer);

            return VELOC_Mem_NSCS;
        }

        MPI_Send(buffer, bytes, MPI_CHAR, destination, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->groupComm);
        toSend -= bytes;
    }

    fclose(fileDesc);
    free(buffer);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It receives checkpint file.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @param      source          Source group rank.
    @param      ptner           0 if receiving Ckpt, 1 if PtnerCkpt.
    @return     integer         VELOC_SUCCESS if successful.

    This function receives Ckpt or PtnerCkpt file from partner proccess.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_RecvCkptFileL2(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                        VELOCT_checkpoint* VELOC_Mem_Ckpt, int source, int ptner) {
    char filename[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    FILE *fileDesc;

    long toRecv;    //remaining data to receive
    if (ptner) { //if want to receive Ptner file
        int ckptID, rank;
        sscanf(VELOC_Mem_Exec->meta[2].ckptFile, "Ckpt%d-Rank%d.velec_mem", &ckptID, &rank);
        sprintf(filename, "%s/Ckpt%d-Pcof%d.velec_mem", VELOC_Mem_Ckpt[2].dir, VELOC_Mem_Exec->ckptID, rank);
        toRecv = VELOC_Mem_Exec->meta[2].pfs[0];
    } else { //if want to receive Ckpt file
        sprintf(filename, "%s/%s", VELOC_Mem_Ckpt[2].dir, VELOC_Mem_Exec->meta[2].ckptFile);
        toRecv = VELOC_Mem_Exec->meta[2].fs[0];
    }

    sprintf(str, "Opening file (wb) (%s) (L2).", filename);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);
    fileDesc = fopen(filename, "wb");
    if (fileDesc == NULL) {
        VELOC_Mem_Print("R2 cannot open the file.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }
    char* buffer = talloc(char, VELOC_Mem_Conf->blockSize);

    while (toRecv > 0) {
        int recvSize = (toRecv > VELOC_Mem_Conf->blockSize) ? VELOC_Mem_Conf->blockSize : toRecv;
        MPI_Recv(buffer, recvSize, MPI_CHAR, source, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->groupComm, MPI_STATUS_IGNORE);
        fwrite(buffer, sizeof(char), recvSize, fileDesc);

        if (ferror(fileDesc)) {
            VELOC_Mem_Print("Error writing the data to the file.", VELOC_Mem_WARN);

            fclose(fileDesc);
            free(buffer);

            return VELOC_Mem_NSCS;
        }

        toRecv -= recvSize;
    }

    fclose(fileDesc);
    free(buffer);

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It recovers L2 ckpt. files using the partner copy.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tries to recover the L2 ckpt. files missing using the
    partner copy. If a ckpt. file and its copy are both missing, then we
    consider this checkpoint unavailable.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_recoverL2(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    int erased[VELOC_Mem_BUFS];
    int i, j; //iterators
    int source = VELOC_Mem_Topo->right; //to receive Ptner file from this process (to recover)
    int destination = VELOC_Mem_Topo->left; //to send Ptner file (to let him recover)
    int res, tres;

    if (mkdir(VELOC_Mem_Ckpt[2].dir, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("Cannot create directory", VELOC_Mem_EROR);
        }
    }

    // Checking erasures
    if (VELOC_Mem_CheckErasures(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, erased) != VELOC_SUCCESS) {
        VELOC_Mem_Print("Error checking erasures.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    i = 0;
    for (j = 0; j < VELOC_Mem_Topo->groupSize * 2; j++) {
        if (erased[j]) {
            i++; // Counting erasures
        }
    }

    if (i == 0) {
        VELOC_Mem_Print("Have all checkpoint files.", VELOC_Mem_DBUG);
        return VELOC_SUCCESS;
    }

    res = VELOC_SUCCESS;
    if (erased[VELOC_Mem_Topo->groupRank] && erased[source + VELOC_Mem_Topo->groupSize]) {
        VELOC_Mem_Print("My checkpoint file and partner copy have been lost", VELOC_Mem_WARN);
        res = VELOC_Mem_NSCS;
    }

    if (erased[VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize] && erased[destination]) {
        VELOC_Mem_Print("My Ptner checkpoint file and his checkpoint file have been lost", VELOC_Mem_WARN);
        res = VELOC_Mem_NSCS;
    }

    MPI_Allreduce(&res, &tres, 1, MPI_INT, MPI_SUM, VELOC_Mem_Exec->groupComm);
    if (tres != VELOC_SUCCESS) {
        return VELOC_Mem_NSCS;
    }

    //recover checkpoint files
    if (VELOC_Mem_Topo->groupRank % 2) {
        if (erased[destination]) { //first send file
            res = VELOC_Mem_SendCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, destination, 1);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }
        if (erased[VELOC_Mem_Topo->groupRank]) { //then receive file
            res = VELOC_Mem_RecvCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, source, 0);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }
    } else {
        if (erased[VELOC_Mem_Topo->groupRank]) { //first receive file
            res = VELOC_Mem_RecvCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, source, 0);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }

        if (erased[destination]) { //then send file
            res = VELOC_Mem_SendCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, destination, 1);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }
    }

    //recover partner files
    if (VELOC_Mem_Topo->groupRank % 2) {
        if (erased[source + VELOC_Mem_Topo->groupSize]) { //fisrst send file
            res = VELOC_Mem_SendCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, source, 0);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }
        if (erased[VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize]) { //receive file
            res = VELOC_Mem_RecvCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, destination, 1);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }
    } else {
        if (erased[VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize]) { //first receive file
            res = VELOC_Mem_RecvCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, destination, 1);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }

        if (erased[source + VELOC_Mem_Topo->groupSize]) { //send file
            res = VELOC_Mem_SendCkptFileL2(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Ckpt, source, 0);
            if (res != VELOC_SUCCESS) {
                return VELOC_Mem_NSCS;
            }
        }
    }

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It recovers L3 ckpt. files ordering the RS decoding algorithm.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tries to recover the L3 ckpt. files missing using the
    RS decoding. If to many files are missing in the group, then we
    consider this checkpoint unavailable.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_recoverL3(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
    int erased[VELOC_Mem_BUFS], j;
    char str[VELOC_Mem_BUFS];


    if (mkdir(VELOC_Mem_Ckpt[3].dir, 0777) == -1) {
        if (errno != EEXIST) {
            VELOC_Mem_Print("Cannot create directory", VELOC_Mem_EROR);
        }
    }

    // Checking erasures
    if (VELOC_Mem_CheckErasures(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, erased) != VELOC_SUCCESS) {
        VELOC_Mem_Print("Error checking erasures.", VELOC_Mem_DBUG);
        return VELOC_Mem_NSCS;
    }

    // Counting erasures
    int l = 0;
    int gs = VELOC_Mem_Topo->groupSize;
    for (j = 0; j < gs; j++) {
        if (erased[j]) {
            l++;
        }
        if (erased[j + gs]) {
            l++;
        }
    }
    if (l > gs) {
        VELOC_Mem_Print("Too many erasures at L3.", VELOC_Mem_DBUG);
        return VELOC_Mem_NSCS;
    }

    // Reed-Solomon decoding
    if (l > 0) {
        sprintf(str, "There are %d encoded/checkpoint files missing in this group.", l);
        VELOC_Mem_Print(str, VELOC_Mem_DBUG);
        if (VELOC_Mem_Decode(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, erased) == VELOC_Mem_NSCS) {
            VELOC_Mem_Print("RS-decoding could not regenerate the missing data.", VELOC_Mem_DBUG);
            return VELOC_Mem_NSCS;
        }
    }

    return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It recovers L4 ckpt. files from the PFS.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tries to recover the ckpt. files using the L4 ckpt. files
    stored in the PFS. If at least one ckpt. file is missing in the PFS, we
    consider this checkpoint unavailable.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_recoverL4(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
   int res;
   if (!VELOC_Mem_Ckpt[4].isInline || VELOC_Mem_Conf->ioMode == VELOC_Mem_IO_POSIX) {
       res = VELOC_Mem_recoverL4Posix(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
   }
   else {
       switch(VELOC_Mem_Conf->ioMode) {
          case VELOC_Mem_IO_MPI:
             res = VELOC_Mem_recoverL4Mpi(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
             break;
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
          case VELOC_Mem_IO_SIONLIB:
             res = VELOC_Mem_recoverL4Sionlib(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt);
             break;
#endif
       }
   }

   return res;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It recovers L4 ckpt. files from the PFS using POSIX.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tries to recover the ckpt. files using the L4 ckpt. files
    stored in the PFS. If at least one ckpt. file is missing in the PFS, we
    consider this checkpoint unavailable.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_recoverL4Posix(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                  VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
   int j, l, gs, erased[VELOC_Mem_BUFS];
   char gfn[VELOC_Mem_BUFS], lfn[VELOC_Mem_BUFS];
   FILE *gfd, *lfd;

   gs = VELOC_Mem_Topo->groupSize;
   if (VELOC_Mem_Topo->nodeRank == 0 || VELOC_Mem_Topo->nodeRank == 1) {
      if (mkdir(VELOC_Mem_Ckpt[1].dir, 0777) == -1) {
         if (errno != EEXIST)
            VELOC_Mem_Print("Directory L1 could NOT be created.", VELOC_Mem_WARN);
      }
   }
   MPI_Barrier(VELOC_Mem_COMM_WORLD);
   // Checking erasures
   if (VELOC_Mem_CheckErasures(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, VELOC_Mem_Ckpt, erased) != VELOC_SUCCESS) {
      VELOC_Mem_Print("Error checking erasures.", VELOC_Mem_DBUG);
      return VELOC_Mem_NSCS;
   }

   l = 0;
   // Counting erasures
   for (j = 0; j < gs; j++) {
      if (erased[j])
         l++;
   }
   if (l > 0) {
      VELOC_Mem_Print("Checkpoint file missing at L4.", VELOC_Mem_DBUG);
      return VELOC_Mem_NSCS;
   }

   // Open files
   sprintf(VELOC_Mem_Exec->meta[1].ckptFile, "Ckpt%d-Rank%d.velec_mem", VELOC_Mem_Exec->ckptID, VELOC_Mem_Topo->myRank);
   sprintf(VELOC_Mem_Exec->meta[4].ckptFile, "Ckpt%d-Rank%d.velec_mem", VELOC_Mem_Exec->ckptID, VELOC_Mem_Topo->myRank);
   sprintf(lfn, "%s/%s", VELOC_Mem_Ckpt[1].dir, VELOC_Mem_Exec->meta[1].ckptFile);
   sprintf(gfn, "%s/%s", VELOC_Mem_Ckpt[4].dir, VELOC_Mem_Exec->meta[4].ckptFile);

   gfd = fopen(gfn, "rb");
   if (gfd == NULL) {
      VELOC_Mem_Print("R4 cannot open the ckpt. file in the PFS.", VELOC_Mem_WARN);
      return VELOC_Mem_NSCS;
   }

   lfd = fopen(lfn, "wb");
   if (lfd == NULL) {
      VELOC_Mem_Print("R4 cannot open the local ckpt. file.", VELOC_Mem_WARN);
      fclose(gfd);
      return VELOC_Mem_NSCS;
   }

   char *readData = talloc(char, VELOC_Mem_Conf->transferSize);
   long bSize = VELOC_Mem_Conf->transferSize;
   long fs = VELOC_Mem_Exec->meta[4].fs[0];
   // Checkpoint files transfer from PFS
   long pos = 0;
   while (pos < fs) {
      if ((fs - pos) < VELOC_Mem_Conf->transferSize) {
         bSize = fs - pos;
      }

      size_t bytes = fread(readData, sizeof(char), bSize, gfd);
      if (ferror(gfd)) {
         VELOC_Mem_Print("R4 cannot read from the ckpt. file in the PFS.", VELOC_Mem_DBUG);

         free(readData);

         fclose(gfd);
         fclose(lfd);

         return  VELOC_Mem_NSCS;
      }

      fwrite(readData, sizeof(char), bytes, lfd);
      if (ferror(lfd)) {
         VELOC_Mem_Print("R4 cannot write to the local ckpt. file.", VELOC_Mem_DBUG);

         free(readData);

         fclose(gfd);
         fclose(lfd);

         return  VELOC_Mem_NSCS;
      }

      pos = pos + bytes;
   }

   free(readData);

   fclose(gfd);
   fclose(lfd);

   return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It recovers L4 ckpt. files from the PFS using MPI-I/O.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tries to recover the ckpt. files using the L4 ckpt. files
    stored in the PFS. If at least one ckpt. file is missing in the PFS, we
    consider this checkpoint unavailable.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_recoverL4Mpi(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
      VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
   int i, buf;
   char gfn[VELOC_Mem_BUFS], lfn[VELOC_Mem_BUFS], mpi_err[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];

   // TODO enable to set stripping unit in the config file (Maybe also other hints)
   // enable collective buffer optimization
   MPI_Info info;
   MPI_Info_create(&info);
   MPI_Info_set(info, "romio_cb_read", "enable");

   // set stripping unit to 4MB
   MPI_Info_set(info, "stripping_unit", "4194304");

   sprintf(VELOC_Mem_Exec->meta[1].ckptFile, "Ckpt%d-Rank%d.velec_mem", VELOC_Mem_Exec->ckptID, VELOC_Mem_Topo->myRank);
   sprintf(VELOC_Mem_Exec->meta[4].ckptFile, "Ckpt%d-mpiio.velec_mem", VELOC_Mem_Exec->ckptID);
   sprintf(lfn, "%s/%s", VELOC_Mem_Ckpt[1].dir, VELOC_Mem_Exec->meta[1].ckptFile);
   sprintf(gfn, "%s/%s", VELOC_Mem_Ckpt[4].dir, VELOC_Mem_Exec->meta[4].ckptFile);

   // open parallel file
   MPI_File pfh;
   buf = MPI_File_open(VELOC_Mem_COMM_WORLD, gfn, MPI_MODE_RDWR, info, &pfh);
   // check if successful
   if (buf != 0) {
      errno = 0;
      MPI_Error_string(buf, mpi_err, NULL);
      if (buf != MPI_ERR_NO_SUCH_FILE) {
         snprintf(str, VELOC_Mem_BUFS, "Unable to access file [MPI ERROR - %i] %s", buf, mpi_err);
         VELOC_Mem_Print(str, VELOC_Mem_EROR);
      }
      return VELOC_Mem_NSCS;
   }

   // create local directories
   if (mkdir(VELOC_Mem_Ckpt[1].dir, 0777) == -1) {
      if (errno != EEXIST) {
          VELOC_Mem_Print("Directory L1 could NOT be created.", VELOC_Mem_WARN);
      }
   }

   // collect chunksizes of other ranks
   MPI_Offset* chunkSizes = talloc(MPI_Offset, VELOC_Mem_Topo->nbApprocs*VELOC_Mem_Topo->nbNodes);
   MPI_Allgather(VELOC_Mem_Exec->meta[4].fs, 1, MPI_OFFSET, chunkSizes, 1, MPI_OFFSET, VELOC_Mem_COMM_WORLD);

   MPI_Offset offset = 0;
   // set file offset
   for (i = 0; i < VELOC_Mem_Topo->splitRank; i++) {
      offset += chunkSizes[i];
   }
   free(chunkSizes);

   FILE *lfd = fopen(lfn, "wb");
   if (lfd == NULL) {
      VELOC_Mem_Print("R4 cannot open the local ckpt. file.", VELOC_Mem_DBUG);
      MPI_File_close(&pfh);
      return VELOC_Mem_NSCS;
   }

   long fs = VELOC_Mem_Exec->meta[4].fs[0];
   char *readData = talloc(char, VELOC_Mem_Conf->transferSize);
   long bSize = VELOC_Mem_Conf->transferSize;
   long pos = 0;
   // Checkpoint files transfer from PFS
   while (pos < fs) {
       if ((fs - pos) < VELOC_Mem_Conf->transferSize) {
           bSize = fs - pos;
       }
      // read block in parallel file
      buf = MPI_File_read_at(pfh, offset, readData, bSize, MPI_BYTE, MPI_STATUS_IGNORE);
      // check if successful
      if (buf != 0) {
         errno = 0;
         MPI_Error_string(buf, mpi_err, NULL);
         snprintf(str, VELOC_Mem_BUFS, "R4 cannot read from the ckpt. file in the PFS. [MPI ERROR - %i] %s", buf, mpi_err);
         VELOC_Mem_Print(str, VELOC_Mem_EROR);
         free(readData);
         MPI_File_close(&pfh);
         fclose(lfd);
         return VELOC_Mem_NSCS;
      }

      fwrite(readData, sizeof(char), bSize, lfd);
      if (ferror(lfd)) {
         VELOC_Mem_Print("R4 cannot write to the local ckpt. file.", VELOC_Mem_DBUG);
         free(readData);
         fclose(lfd);
         MPI_File_close(&pfh);
         return  VELOC_Mem_NSCS;
      }

      offset += bSize;
      pos = pos + bSize;
   }

   free(readData);
   fclose(lfd);

   if (MPI_File_close(&pfh) != 0) {
      VELOC_Mem_Print("Cannot close MPI file.", VELOC_Mem_WARN);
      return VELOC_Mem_NSCS;
   }

   return VELOC_SUCCESS;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It recovers L4 ckpt. files from the PFS using SIONlib.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      VELOC_Mem_Ckpt        Checkpoint metadata.
    @return     integer         VELOC_SUCCESS if successful.

    This function tries to recover the ckpt. files using the L4 ckpt. files
    stored in the PFS. If at least one ckpt. file is missing in the PFS, we
    consider this checkpoint unavailable.

 **/
/*-------------------------------------------------------------------------*/
#ifdef ENABLE_SIONLIB // --> If SIONlib is installed
int VELOC_Mem_recoverL4Sionlib(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
      VELOCT_topology* VELOC_Mem_Topo, VELOCT_checkpoint* VELOC_Mem_Ckpt)
{
   int res;
   char str[VELOC_Mem_BUFS], gfn[VELOC_Mem_BUFS], lfn[VELOC_Mem_BUFS];

   if (mkdir(VELOC_Mem_Ckpt[1].dir, 0777) == -1) {
       if (errno != EEXIST) {
           VELOC_Mem_Print("Directory L1 could NOT be created.", VELOC_Mem_WARN);
       }
   }

   sprintf(VELOC_Mem_Exec->meta[1].ckptFile, "Ckpt%d-Rank%d.velec_mem", VELOC_Mem_Exec->ckptID, VELOC_Mem_Topo->myRank);
   sprintf(VELOC_Mem_Exec->meta[4].ckptFile, "Ckpt%d-sionlib.velec_mem", VELOC_Mem_Exec->ckptID);
   sprintf(lfn, "%s/%s", VELOC_Mem_Ckpt[1].dir, VELOC_Mem_Exec->meta[1].ckptFile);
   sprintf(gfn, "%s/%s", VELOC_Mem_Ckpt[4].dir, VELOC_Mem_Exec->meta[4].ckptFile);

   // this is done, since sionlib aborts if the file is not readable.
   if (access(gfn, F_OK) != 0) {
      return VELOC_Mem_NSCS;
   }

   int numFiles = 1;
   int nlocaltasks = 1;
   int* file_map = calloc(1, sizeof(int));
   int* ranks = talloc(int, 1);
   int* rank_map = talloc(int, 1);
   sion_int64* chunkSizes = talloc(sion_int64, 1);
   int fsblksize = -1;
   chunkSizes[0] = VELOC_Mem_Exec->meta[4].fs[0];
   ranks[0] = VELOC_Mem_Topo->splitRank;
   rank_map[0] = VELOC_Mem_Topo->splitRank;
   int sid = sion_paropen_mapped_mpi(gfn, "rb,posix", &numFiles, VELOC_Mem_COMM_WORLD, &nlocaltasks, &ranks, &chunkSizes, &file_map, &rank_map, &fsblksize, NULL);

   FILE* lfd = fopen(lfn, "wb");
   if (lfd == NULL) {
      VELOC_Mem_Print("R4 cannot open the local ckpt. file.", VELOC_Mem_DBUG);
      sion_parclose_mapped_mpi(sid);
      free(file_map);
      free(ranks);
      free(rank_map);
      free(chunkSizes);
      return VELOC_Mem_NSCS;
   }

   res = sion_seek(sid, VELOC_Mem_Topo->splitRank, SION_CURRENT_BLK, SION_CURRENT_POS);
   // check if successful
   if (res != SION_SUCCESS) {
      VELOC_Mem_Print("SIONlib: Could not set file pointer", VELOC_Mem_EROR);
      sion_parclose_mapped_mpi(sid);
      free(file_map);
      free(ranks);
      free(rank_map);
      free(chunkSizes);
      fclose(lfd);
      return VELOC_Mem_NSCS;
   }

   // Checkpoint files transfer from PFS
   while (!sion_feof(sid)) {
      long fs = VELOC_Mem_Exec->meta[4].fs[0];
      char *readData = talloc(char, VELOC_Mem_Conf->transferSize);
      long bSize = VELOC_Mem_Conf->transferSize;
      long pos = 0;
      // Checkpoint files transfer from PFS
      while (pos < fs) {
         if ((fs - pos) < VELOC_Mem_Conf->transferSize) {
             bSize = fs - pos;
         }
         res = sion_fread(readData, sizeof(char), bSize, sid);
         if (res != bSize) {
            sprintf(str, "SIONlib: Unable to read %lu Bytes from file", bSize);
            VELOC_Mem_Print(str, VELOC_Mem_EROR);
            sion_parclose_mapped_mpi(sid);
            free(file_map);
            free(ranks);
            free(rank_map);
            free(chunkSizes);
            free(readData);
            fclose(lfd);
            return VELOC_Mem_NSCS;
         }

         fwrite(readData, sizeof(char), bSize, lfd);
         if (ferror(lfd)) {
            VELOC_Mem_Print("R4 cannot write to the local ckpt. file.", VELOC_Mem_DBUG);
            free(readData);
            fclose(lfd);
            sion_parclose_mapped_mpi(sid);
            free(file_map);
            free(ranks);
            free(rank_map);
            free(chunkSizes);
            return  VELOC_Mem_NSCS;
         }

         pos = pos + bSize;
      }
      free(readData);
   }

   fclose(lfd);

   sion_parclose_mapped_mpi(sid);
   free(file_map);
   free(ranks);
   free(rank_map);
   free(chunkSizes);

   return VELOC_SUCCESS;
}
#endif
