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
#define ITER_TIMES  5000
#define ITER_OUT    500
#define WORKTAG     50
#define REDUCE      5

void initData(int nbLines, int M, int rank, double *h);
double doWork(int numprocs, int rank, int M, int nbLines, double *g, double *h);
