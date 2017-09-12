/**
 *  @file   heatdis.h
 *  @author Leonardo A. Bautista Gomez
 *  @date   May, 2014
 *  @brief  Heat distribution code to test VeloC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <veloc.h>
#include "mpi.h"


#define PRECISION   0.005
#define ITER_TIMES  500
#define ITER_OUT    50
#define WORKTAG     5
#define REDUCE      1

void initData(int nbLines, int M, int rank, double *h);
double doWork(int numprocs, int rank, int M, int nbLines, double *g, double *h);
