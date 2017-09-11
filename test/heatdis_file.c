/**
 *  @file   heatdis.c
 *  @author Leonardo A. Bautista Gomez
 *  @date   May, 2014
 *  @brief  Heat distribution code to test VELOC.
 */

#include "heatdis.h"

int main(int argc, char *argv[])
{
    int rank, nbProcs, nbLines, i, M, arg;
    double wtime, *h, *g, memSize, localerror, globalerror = 1;

    MPI_Init(&argc, &argv);
    VELOC_Init(argv[2]);

    MPI_Comm_size(MPI_COMM_WORLD, &nbProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    arg = atoi(argv[1]);
    M = (int)sqrt((double)(arg * 1024.0 * 512.0 * nbProcs)/sizeof(double));
    nbLines = (M / nbProcs)+3;
    h = (double *) malloc(sizeof(double *) * M * nbLines);
    g = (double *) malloc(sizeof(double *) * M * nbLines);
    initData(nbLines, M, rank, g);
    memSize = M * nbLines * 2 * sizeof(double) / (1024 * 1024);

    if (rank == 0) printf("Local data size is %d x %d = %f MB (%d).\n", M, nbLines, memSize, arg);
    if (rank == 0) printf("Target precision : %f \n", PRECISION);
    if (rank == 0) printf("Maximum number of iterations : %d \n", ITER_TIMES);

    // init loop counter (before restart which may overwrite it)
    i = 0;

    // read checkpoint if we're restarting
    int flag;
    VELOC_Restart_test(&flag);
    if (flag) {
        VELOC_Restart_begin();

        // build file name for this rank
        char file[1024];
        snprintf(file, sizeof(file), "ckpt.%d", rank);

        // get path to file from VELOC
        char veloc_file[VELOC_MAX_NAME];
        VELOC_Route_file(file, veloc_file);

        // open file for writing
        int valid = 1;
        FILE* fd = fopen(veloc_file, "rb");
        if (fd != NULL) {
            if (fread(&i, sizeof(int),            1, fd) != 1)         { valid = 0; }
            if (fread( h, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
            if (fread( g, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
            fclose(fd);
        } else {
            // failed to open file
            valid = 0;
        }

        VELOC_Restart_end(valid);
    }

    wtime = MPI_Wtime();
    while(i < ITER_TIMES)
    {
        // write checkpoint if needed
        int flag;
        VELOC_Checkpoint_test(&flag);
        if (flag) {
            VELOC_Checkpoint_begin();

            // build file name for this rank
            char file[1024];
            snprintf(file, sizeof(file), "time.%d/ckpt.%d", i, rank);

            // get path to file from VELOC
            char veloc_file[VELOC_MAX_NAME];
            VELOC_Route_file(file, veloc_file);

            // open file for writing
            int valid = 1;
            FILE* fd = fopen(veloc_file, "wb");
            if (fd != NULL) {
                if (fwrite(&i, sizeof(int),            1, fd) != 1)         { valid = 0; }
                if (fwrite( h, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
                if (fwrite( g, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
                fclose(fd);
            } else {
                // failed to open file
                valid = 0;
            }

            VELOC_Checkpoint_end(valid);
        }

        localerror = doWork(nbProcs, rank, M, nbLines, g, h);
        if (((i%ITER_OUT) == 0) && (rank == 0)) printf("Step : %d, error = %f\n", i, globalerror);
        if ((i%REDUCE) == 0) MPI_Allreduce(&localerror, &globalerror, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        if(globalerror < PRECISION) break;
        i++;
    }
    if (rank == 0) printf("Execution finished in %lf seconds.\n", MPI_Wtime() - wtime);

    free(h);
    free(g);
    VELOC_Finalize();
    MPI_Finalize();
    return 0;
}


