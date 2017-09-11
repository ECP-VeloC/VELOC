/**
 *  @file   heatdis_common.c
 *  @author Leonardo A. Bautista Gomez
 *  @date   May, 2014
 *  @brief  Heat distribution code to test VeloC.
 */

#include "heatdis.h"

void initData(int nbLines, int M, int rank, double *h)
{
    int i, j;
    for (i = 0; i < nbLines; i++)
    {
        for (j = 0; j < M; j++)
        {
            h[(i*M)+j] = 0;
        }
    }
    if (rank == 0)
    {
        for (j = (M*0.1); j < (M*0.9); j++)
        {
            h[j] = 100;
        }
    }
}


double doWork(int numprocs, int rank, int M, int nbLines, double *g, double *h)
{
    int i,j;
    MPI_Request req1[2], req2[2];
    MPI_Status status1[2], status2[2];
    double localerror;
    localerror = 0;
    for(i = 0; i < nbLines; i++)
    {
        for(j = 0; j < M; j++)
        {
            h[(i*M)+j] = g[(i*M)+j];
        }
    }
    if (rank > 0)
    {
        MPI_Isend(g+M, M, MPI_DOUBLE, rank-1, WORKTAG, MPI_COMM_WORLD, &req1[0]);
        MPI_Irecv(h,   M, MPI_DOUBLE, rank-1, WORKTAG, MPI_COMM_WORLD, &req1[1]);
    }
    if (rank < numprocs-1)
    {
        MPI_Isend(g+((nbLines-2)*M), M, MPI_DOUBLE, rank+1, WORKTAG, MPI_COMM_WORLD, &req2[0]);
        MPI_Irecv(h+((nbLines-1)*M), M, MPI_DOUBLE, rank+1, WORKTAG, MPI_COMM_WORLD, &req2[1]);
    }
    if (rank > 0)
    {
        MPI_Waitall(2,req1,status1);
    }
    if (rank < numprocs-1)
    {
        MPI_Waitall(2,req2,status2);
    }
    for(i = 1; i < (nbLines-1); i++)
    {
        for(j = 0; j < M; j++)
        {
            g[(i*M)+j] = 0.25*(h[((i-1)*M)+j]+h[((i+1)*M)+j]+h[(i*M)+j-1]+h[(i*M)+j+1]);
            if(localerror < fabs(g[(i*M)+j] - h[(i*M)+j]))
            {
                localerror = fabs(g[(i*M)+j] - h[(i*M)+j]);
            }
        }
    }
    if (rank == (numprocs-1))
    {
        for(j = 0; j < M; j++)
        {
            g[((nbLines-1)*M)+j] = g[((nbLines-2)*M)+j];
        }
    }
    return localerror;
}

