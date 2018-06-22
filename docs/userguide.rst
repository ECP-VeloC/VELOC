.. raw:: latex

   \vspace*{1.0in}

.. raw:: html

   <div style="color: gray">

--------------

.. raw:: html

   </div>

VELOC User Guide
================

.. raw:: html

   <div style="color: gray">

--------------

.. raw:: html

   </div>

.. raw:: latex

   \vspace*{10.0ex}

.. raw:: latex

   \Large

.. raw:: latex

   \vspace*{2.0ex}

Revision: VELOC Revision 0.5

.. raw:: latex

   \normalsize

.. raw:: latex

   \setcounter{page}{0}

.. raw:: latex

   \thispagestyle{empty}

.. raw:: latex

   \newpage

.. raw:: latex

   \tableofcontents

.. _ch:users:

Who should use this document?
-----------------------------

| This document is a draft version and is subject to changes.
| This document is intended for users who wish to use the VELOC software
  for checkpoint restart. The document discusses how to install,
  configure and test the software with packaged examples.

.. _ch:velocsetup:

Setting up the VELOC software
-----------------------------

This chapter demonstrates how to setup the VELOC software. More
specifically, it describes how to download, install and compile the
VELOC code.

Downloading the VELOC software
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The VELOC software is currently under development. It is available in a
git repo at https://xgitlab.cels.anl.gov/ecp-veloc/ftiscr. To get access
to code, please contact the developers of VELOC.

The software can be downloaded by using the following command. This will
download the code directory in your current directory.

*git clone*\ https://xgitlab.cels.anl.gov/ecp-veloc/veloc

Installing the VELOC software
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Execute the ``./buildme`` script to build and install VELOC. This will
create two directories ``build`` and ``install``.

The current VELOC library is implemented on top of SCR. The ``buildme``
script downloads and installs SCR and its dependency PDSH. It then
builds the VELOC library and several example programs.

Executing the VELOC examples
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To run the examples, move to the ``install`` directory and run
``make test``.

Currently, VELOC tests a simple heat distribution example. There are 3
versions of the example, to test the 3 modes of operation of VELOC:

-  Memory interface (similar to that of FTI),
-  File interface (similar to that of SCR),
-  Both the memory and file interface.

Each version of the example creates 3 CMake tests:

-  one to start the program,
-  one to test the restart the program,
-  and one to cleanup the files from the test.

These tests are run serially.

VELOC Configuration
~~~~~~~~~~~~~~~~~~~

The current VELOC configuration file is the same as that used for SCR. A
sample configuration file can be found at ``test/test.conf``.

.. _ch:examples:

Examples using VELOC API
------------------------

This chapter demonstrates how to use the VELOC API through a few
examples. These examples can also be found in the VELOC software in the
examples directory.

Heat Equation Example
~~~~~~~~~~~~~~~~~~~~~

An example of how the VELOC API can be used is seen in the following
code. This example program implements a distributed heat equation
calculation. Here, we see the use of both FTI-style memory protection
and SCR-style file protection.

.. code:: c

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

       // show that we can mix by saving i and h via protect
       // write g to a file, e.g., consider a program in which
       // one library uses protect and another library uses files

       VELOC_Mem_protect(0, &i,         1, VELOC_INTG);
       VELOC_Mem_protect(1,  h, M*nbLines, VELOC_DBLE);

       // init loop counter (before restart which may overwrite it)
       i = 0;

       // read checkpoint if we're restarting
       int flag;
       VELOC_Restart_test(&flag);
       if (flag) {
           char ckptname[VELOC_MAX_NAME];
           VELOC_Restart_begin(ckptname);

           // build file name for this rank for memory
           char file[1024];
           snprintf(file, sizeof(file), "mem.%d", rank);

           // restore protected variables
           VELOC_Restart_mem(file, VELOC_RECOVER_ALL, NULL, 0);

           // build file name for this rank
           snprintf(file, sizeof(file), "ckpt.%d", rank);

           // get path to file from VELOC
           char veloc_file[VELOC_MAX_NAME];
           VELOC_Route_file(file, veloc_file);

           // open file for writing
           int valid = 1;
           FILE* fd = fopen(veloc_file, "rb");
           if (fd != NULL) {
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
               // define a name for this checkpoint
               char ckptname[VELOC_MAX_NAME];
               snprintf(ckptname, sizeof(ckptname), "time.%d", i);

               VELOC_Checkpoint_begin(ckptname);

               // build file name for this rank for memory
               char file[1024];
               snprintf(file, sizeof(file), "time.%d/mem.%d", i, rank);

               // save protected variables
               VELOC_Checkpoint_mem(file);

               // build file name for this rank
               snprintf(file, sizeof(file), "time.%d/ckpt.%d", i, rank);

               // get path to file from VELOC
               char veloc_file[VELOC_MAX_NAME];
               VELOC_Route_file(file, veloc_file);

               // open file for writing
               int valid = 1;
               FILE* fd = fopen(veloc_file, "wb");
               if (fd != NULL) {
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
