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

	if(argc < 2)
	{
		printf("Test case: heatdis_mem [data_size (in MB)] [configuration_file]\n");
		printf("Example: heatdis_mem 10 config.vec\n");
		exit(0);
	}

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

    VELOC_Mem_protect(0, &i, 1, VELOC_INTG);
    VELOC_Mem_protect(1, h, M*nbLines, VELOC_DBLE);
    VELOC_Mem_protect(2, g, M*nbLines, VELOC_DBLE);

    wtime = MPI_Wtime();
    for(i = 0; i < ITER_TIMES; i++)
    {
        int checkpointed = VELOC_Mem_snapshot();
        localerror = doWork(nbProcs, rank, M, nbLines, g, h);
        if (((i%ITER_OUT) == 0) && (rank == 0)) printf("Step : %d, error = %f\n", i, globalerror);
        if ((i%REDUCE) == 0) MPI_Allreduce(&localerror, &globalerror, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        if(globalerror < PRECISION) break;
    }
    if (rank == 0) printf("Execution finished in %lf seconds.\n", MPI_Wtime() - wtime);

    free(h);
    free(g);
    VELOC_Finalize();
    MPI_Finalize();
    return 0;
}


