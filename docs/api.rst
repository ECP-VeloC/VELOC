VeloC API
=========

This document is intended for application developers that need to
integrate VeloC into their application code. It focuses on the API
that VeloC exposes for this purpose.

.. _ch:veloc_client_api:

API Specifications
------------------

VeloC supports two modes of operation: memory-based and file-based
checkpoints. With memory-based checkpoints, an application registers regions
of its memory that should be saved with each checkpoint and restored upon a restart. 
In this mode, the serialization of the memory regions happens automatically.
With file-based checkpoints, the application has full control over how to serialize
the critical data structures needed for restart into checkpoint files.

Note: the most up-to-date API specification is found in the VELOC header file:
``<install_dir>/include/veloc.h``

Return Codes
~~~~~~~~~~~~

All functions use the following return codes, defined as an ``int``
type.

-  ``VELOC_SUCCESS``: The function completed successfully.
-  ``VELOC_FAILURE``: Indicates a failure. VeloC prints a corresponding error message when this error code is returned. In the future, more error codes may be added to indicate common error scenarios that can be used by the application to take further action.

Initializing and Finalizing VeloC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Initialization
^^^^^^^^^^^^^^

::

   int VELOC_Init(IN MPI_Comm comm, IN const char *cfg_file)

ARGUMENTS
'''''''''

- **comm**: The MPI communicator corresponding to the processes that need to checkpoint/restart as a group (typically MPI_COMM_WORLD)
- **cfg_file**: The VeloC configuration file, detailed in the user guide.

DESCRIPTION
'''''''''''

This function initializes the VELOC library. It must be called collectively by all processes before any other VELOC function. A good practice is to call it immediately after ``MPI_Init()``. 

Non-collective initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Init_single(unsigned int unique_id, IN const char *cfg_file)

ARGUMENTS
'''''''''

- **unique_id**: A unique identifier of the process that needs to checkpoint individually
- **cfg_file**: The VeloC configuration file, detailed in the user guide.

DESCRIPTION
'''''''''''

This function initializes the VELOC library in non-collective mode, which enables each process to checkpoint and restart independently of the other processes. Notably, this will impact the behavior of ``VELOC_Restart_test`` (detailed below), which will return the latest version available for the calling process, rather than the whole group.

Finalize
^^^^^^^^

::

   int VELOC_Finalize(IN int drain)

ARGUMENTS
'''''''''

- **drain**: a bool flag specifying whether to wait for the active backend to flush any pending checkpoints to persistent storage (non-zero) or to finalize immediately (0).

.. _description-1:

DESCRIPTION
'''''''''''

This function shuts down the VELOC library. It must be called collectively by all processes and no other VELOC function is allowed afterwards. A good practice is to call it immediately before ``MPI_Finalize()``.

Memory-Based Mode
~~~~~~~~~~~~~~~~~

In memory-based mode, applications need to register any critical memory regions needed for restart. Registration is allowed at any moment before initiating a checkpoint or restart. Memory regions can also be unregistered if they become non-critical at any moment during runtime.

.. _memory-registration-1:

Memory Registration
^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Mem_protect(IN int id, IN void * ptr, IN size_t count, IN size_t base_size)
   
.. _arguments-2:

ARGUMENTS
'''''''''

-  **id**: An application defined id to identify the memory region
-  **ptr**: A pointer to the beginning of the memory region.
-  **count**: The number of elements in the memory region.
-  **base_size**: The size of each element in the memory region.
   
.. _description-3:

DESCRIPTION
'''''''''''

This function registers a memory region for checkpoint/restart. Each process can register and unregister its own 
memory regions independently of the other processes. The id of the memory region must be unique within 
each process. 

Memory Deregistration
^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Mem_unprotect(IN int id)

.. _arguments-3:

ARGUMENTS
'''''''''

-  **id**: The id of the memory region previously registered with ``VELOC_Mem_protect``

.. _description-4:

DESCRIPTION
'''''''''''

This function deregisters a memory region for checkpoint/restart. 

File-Based Mode
~~~~~~~~~~~~~~~

In the file-based mode, applications need to manually serialize/recover the critical data structures to/from 
checkpoint files. This mode provides fine-grain control over the serialization process and is especially useful when the
application uses non-contiguous memory regions for which the memory-based API is not convenient to use.

.. _file-registration-1:

File Registration
^^^^^^^^^^^^^^^^^

::

   int VELOC_Route_file(IN char *original_name, OUT char *ckpt_file_name)
   
.. _arguments-4:

ARGUMENTS
'''''''''

- **ckpt_file_name**: The name of the checkpoint file that the user needs to use to perform I/O
- **original_name**: The original name of the checkpoint file. VELOC will use **ckpt_file_name** internally 
but will stick to the original name when persisting the checkpoint on the parallel file system. This enables
users to customize the checkpoint namespace to facilitate their use for other purposes than restart (e.g. analytics).

.. _description-5:

DESCRIPTION
'''''''''''

To enable the file-based mode, each process needs to use a predefined checkpoint file name that is obtained from VeloC.
Unlike the memory-based mode, this function needs to be called after beginning the checkpoint/restart phase (detailed
below). The process then opens the file, reads or writes the critical data structures depending on whether it performs 
a checkpoint or restart, then closes the file and then ends the checkpoint/restart phase (detailed below).

Checkpoint Functions
~~~~~~~~~~~~~~~~~~~~

Begin Checkpoint Phase
^^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Checkpoint_begin(IN const char * name, int version)

.. _arguments-6:

ARGUMENTS
'''''''''

-  **name**: The label of the checkpoint.
-  **version**: The version of the checkpoint, needs to increase with each checkpoint (e.g. iteration number)    

.. _description-7:

DESCRIPTION
'''''''''''

This function begins the checkpoint phase. It must be called collectively by all processes within the 
same checkpoint/restart group. The name must be an alphanumeric string holding letters and numbers only.

Serialize Memory Regions
^^^^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Checkpoint_mem()

.. _arguments-7:

ARGUMENTS
'''''''''

-  None

.. _description-8:

DESCRIPTION
'''''''''''

The function writes the memory regions previously registered in memory-based mode to the local checkpoint file 
corresponding to each process. It must be called after beginning the checkpoint/restart phase and before ending it.

Close Checkpoint Phase
^^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Checkpoint_end(IN int success)

.. _arguments-8:

ARGUMENTS
'''''''''

-  **success**: Bool flag indicating whether the calling process completed its checkpoint successfully.

.. _description-9:

DESCRIPTION
'''''''''''

This function ends the checkpoint phase. It must be called collectively by all processes within the 
same checkpoint/restart group. The success flag indicates to VeloC whether the process has successfuly managed
to write the local checkpoint. In synchronous mode, ending the checkpoint phase will perform all resilience strategies
employed by VeloC in blocking fashion. The return value indicates whether these strategies succeeded or not. In 
asynchornous mode, ending the checkpoint phase will trigger all resilience strategies in the background, while 
returning control to the application immediately. This operation is always succesful.

Wait for Checkpoint Completion
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    int VELOC_Checkpoint_wait()   
    
.. _arguments-9:

ARGUMENTS
'''''''''
- None

.. _description-10:

DESCRIPTION
'''''''''''

This routine waits for any resilience strategies employed by VeloC in the background to finish. The return value 
indicates whether they were successful or not. The function is meaningul only in asynchronous mode. It has no effect 
in synchronous mode and simply returns success.

Convenience Checkpoint Wrapper
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    int VELOC_Checkpoint(IN const char *name, int version)   
    
.. _arguments-9:

ARGUMENTS
'''''''''
-  **name**: The label of the checkpoint.
-  **version**: The version of the checkpoint, needs to increase with each checkpoint (e.g. iteration number) 

.. _description-10:

DESCRIPTION
'''''''''''
This function is a convenience wrapper equivalent with waiting for the previous checkpoint (if in asynchronous mode), 
then starting a new checkpoint phase, writing all registered memory regions and closing the checkpoint phase. 

Restart Functions
~~~~~~~~~~~~~~~~~

Obtain latest version
^^^^^^^^^^^^^^^^^^^^^

::

    int VELOC_Restart_test(IN const char *name, IN int version)

.. _arguments-9:

ARGUMENTS
'''''''''
- **name** : Label of the checkpoint
- **max_ver** : Maximum version to restart from

.. _description-10:

DESCRIPTION
'''''''''''

This function probes for the most recent version less than **max_ver** that can be used to restart from. If no upper 
limit is desired, **max_ver** can be set to zero to probe for the most recent version. Specifying an upper limit is 
useful when the most recent version is corrupted (e.g. the restored data structures fail integrity checks) and a new 
restart is needed based on the preceding version. The application can repeat the process until a valid version is found 
or no more previous versions are available. The function returns VELOC_FAILURE if no version is available or a positive
integer representing the most recent version otherwise.

Open Restart Phase
^^^^^^^^^^^^^^^^^^

::

    int VELOC_Restart_begin(IN const char *name, IN int version)

.. _arguments-10:

ARGUMENTS
'''''''''

- **name** : Label of the checkpoint
- **version** :  Version of the checkpoint

.. _description-11:

DESCRIPTION
'''''''''''

This function begins the restart phase. It must be called collectively by all processes within the 
same checkpoint/restart group. The version of the checkpoint can be either the version returned by ``VELOC_Restart_test``
or any other lower version that is available.

Memory-based Restart
^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Recover_selective(IN int mode, INT int *ids, IN int length)

ARGUMENTS
'''''''''

- **mode** : One of VELOC_RECOVER_ALL (all regions from the checkpoint, ignores rest of arguments), VELOC_RECOVER_SOME (regions explicitly specified in ids), VELOC_RECOVER_REST (all regions except those specified in ids)
- **ids** :  Array of ids corresponding to the memory regions previously saved in the checkpoint
- **length**: Numer of elements in array of ids

DESCRIPTION
'''''''''''

This function restores the memory regions from the checkpoint speficied when calling ``VELOC_Restart_begin()``. Must be called between ``VELOC_Restart_begin()`` and ``VELOC_Restart_end()``. For all ids that will be restored, a previous call to ``VELOC_Mem_protect()`` must have been issued. The size of the registered memory region must be large enough to fit the data from the checkpoint. A typical use of this function relies on VELOC_RECOVER_SOME to figure out the size of data structures (assumed to be saved into the checkpoint), allocate and protect memory regions large enough to hold them, the use VELOC_RECOVER_REST to restore the content.

::

   int VELOC_Recover_mem()

.. _arguments-11:

ARGUMENTS
'''''''''

-  None

.. _description-12:

DESCRIPTION
'''''''''''

This is a convenience wrapper equivalent to calling ``VELOC_Recover_selective(VELOC_RECOVER_ALL, NULL, 0)``

Close Restart Phase
^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Restart_end (IN int success)

.. _arguments-12:

ARGUMENTS
'''''''''

-  **sucess**: Bool flag indicating whether the calling process restored its state from the checkpoint successfully.

.. _description-13:

DESCRIPTION
'''''''''''

This function ends the restart phase. It must be called collectively by all processes within the 
same checkpoint/restart group. The success flag indicates to VeloC whether the process has successfuly managed
to restore the cricial data structures from the checkpoint specified in ``VELOC_Restart_begin()``. 

Convenience Restart Wrapper
^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    int VELOC_Restart(IN const char *name, IN int version)

.. _arguments-10:

ARGUMENTS
'''''''''

- **name** : Label of the checkpoint
- **version** :  Version of the checkpoint

.. _description-11:

DESCRIPTION
'''''''''''

This function is a convenience wrapper for opening a new restart phase, recovering the registered memory regions from the
checkpoint and closing the restart phase.

.. _ch:veloc_example:

Example
-------

To illustrate the API, we have included with VeloC a sample MPI application that simulates the propagation of heat in a
medium. This application can be found in the ``test`` sub-directory and includes both the original and two modified versions
that use VeloC: one using the memory-based API (``heatdis_mem``) and the other using the file-based API (``headis_file``).

Original Code
~~~~~~~~~~~~~

In a nutshell, the original heatdis application has the following basic structure:

::

    MPI_Init(&argc, &argv);
    // further initialization code
    // allocate two critical double arrays of size M
    h = (double *) malloc(sizeof(double *) * M * nbLines);
    g = (double *) malloc(sizeof(double *) * M * nbLines);
    // set the number of iterations to 0
    i = 0;
    while (i < n) {
        // iteratively compute the heat distribution
        // increment the number of iterations
        i++;
    }
    MPI_Finalize();

Memory-based API
~~~~~~~~~~~~~~~~

To add checkpoint/restart functionality using VeloC in memory-based mode, several modifications are necessary: 
(1) initialize VeloC (immediately after ``MPI_Init``); (2) register the memory regions corresponding to the critical arrays; 
(3) check if there is a previous checkpoint to restart from using ``VeloC_Restart_test``; (4) if yes, restore the memory
regions to their initial state; (5) every K iterations initiate a checkpoint; (6) finalize VeloC before calling ``MPI_Finalize``. This is illustrated below:

:: 

   MPI_Init(&argc, &argv);
   VELOC_Init(MPI_COMM_WORLD, argv[2]); // (1): init
   // further initialization code
   // allocate two critical double arrays of size M
   h = (double *) malloc(sizeof(double *) * M * nbLines);
   g = (double *) malloc(sizeof(double *) * M * nbLines);
   // (2): protect
   VELOC_Mem_protect(0, &i, 1, sizeof(int));
   VELOC_Mem_protect(1, h, M * nbLines, sizeof(double));
   VELOC_Mem_protect(2, g, M * nbLines, sizeof(double));
   // (3): check for previous checkpoint version
   int v = VELOC_Restart_test("heatdis", 0);
   // (4): restore memory content if previous version found
   if (v > 0) {
       printf("Previous checkpoint found at iteration %d, initiating restart...\n", v);
       // v can be any version, independent of what VELOC_Restart_test is returning
       assert(VELOC_Restart("heatdis", v) == VELOC_SUCCESS);
    } else
        i = 0;
    while (i < n) {
        // iteratively compute the heat distribution
        // (5): checkpoint every K iterations
        if (i % K == 0)
            assert(VELOC_Checkpoint("heatdis", i) == VELOC_SUCCESS);
         // increment the number of iterations
         i++;
    }
    VELOC_Finalize(0); // (6): finalize
    MPI_Finalize();

File-based API
~~~~~~~~~~~~~~

To add checkpoint/restart functionality using VeloC in file-based mode, the same modifications are needed as in the case of
memory-based API mode, except for the checkpoint and restart, which need to be manually implemented:

Checkpoint
^^^^^^^^^^
::

    if (i % K == 0) {
        assert(VELOC_Checkpoint_wait() == VELOC_SUCCESS);
        assert(VELOC_Checkpoint_begin("heatdis", i) == VELOC_SUCCESS);
        char veloc_file[VELOC_MAX_NAME];
        assert(VELOC_Route_file(veloc_file) == VELOC_SUCCESS);
        int valid = 1;
        FILE* fd = fopen(veloc_file, "wb");
        if (fd != NULL) {
            if (fwrite(&i, sizeof(int),            1, fd) != 1)         { valid = 0; }
            if (fwrite( h, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
            if (fwrite( g, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
            fclose(fd);
        } else 
            // failed to open file
            valid = 0;
        assert(VELOC_Checkpoint_end(valid) == VELOC_SUCCESS);
    }

Restart
^^^^^^^
:: 

    assert(VELOC_Restart_begin("heatdis", v) == VELOC_SUCCESS);
    char veloc_file[VELOC_MAX_NAME];
    assert(VELOC_Route_file(veloc_file) == VELOC_SUCCESS);
    int valid = 1;
    FILE* fd = fopen(veloc_file, "rb");
    if (fd != NULL) {
        if (fread(&i, sizeof(int),            1, fd) != 1)         { valid = 0; }
        if (fread( h, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
        if (fread( g, sizeof(double), M*nbLines, fd) != M*nbLines) { valid = 0; }
        fclose(fd);
    } else
        // failed to open file
        valid = 0;
    assert(VELOC_Restart_end(valid) == VELOC_SUCCESS);

