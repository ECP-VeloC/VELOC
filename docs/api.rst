.. raw:: latex

   \vspace*{1.0in}

.. raw:: html

   <div style="color: gray">

--------------

.. raw:: html

   </div>

VELOC Software API
==================

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

Revision: VELOC API Version 1.0 - Document revision 0.8

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

.. _ch:audience:

Who should use this document?
-----------------------------

| This document is a draft version and is subject to changes.
| This document is intended for users who wish to use the VELOC software
  for checkpoint restart. The document discusses the VELOC API.


.. _ch:veloc_client_api:

VELOC API
---------

This chapter describes routines that are a part of the VELOC Client
Interface. **NOTE:** We may add more routines that enable developers
finer control of VELOC.

VELOC supports two fundamental checkpoint abstractions: memory-based
checkpoints and file-based checkpoints. With memory-based checkpoints,
an application registers portions of its memory that should be saved
with each checkpoint and restored upon a restart. During a checkpoint,
the VELOC library serializes these memory regions and stores the state
to files. In file-based checkpoints, the application serializes its
state to files, and it registers those files with VELOC. An application
is free to use either abstraction, and both methods can be used in
combination if desired.

Version Information
~~~~~~~~~~~~~~~~~~~

The VELOC header file "veloc.h" defines macros that describe the version
of the software.

-  ``VELOC_VERSION_MAJOR``: a number
-  ``VELOC_VERSION_MINOR``: a number
-  ``VELOC_VERSION_PATCH``: a number
-  ``VELOC_VERSION``: a version string, such as “v0.0.1”

Return Codes
~~~~~~~~~~~~

All functions use the following return codes, defined as an ``int``
type.

-  ``VELOC_SUCCESS``: The function completed successfully.
-  ``VELOC_ERROR``: Indicates an error. This code will be expanded
   later. Specific error codes could include memory errors, a missing
   configuration file, non-exist ant directories, missing checkpoint
   files, or not enough available storage.

Initializing and Finalizing the VELOC library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Initialization
^^^^^^^^^^^^^^

::

   int VELOC_Init(
      IN MPI_Comm comm, 
      IN const char *cfg_file
   )

ARGUMENT
''''''''
- **comm**: the MPI communicator
- **cfg_file**: configuration file with the following fields:
   scratch = <path> (node-local path where VELOC can save temporary checkpoints that live for the duration of the reservation)
   persistent = <path> (persistent path where VELOC can save durable checkpoints that live indefinitely) 
    

DESCRIPTION
'''''''''''

This function initializes the VELOC library. It must be called before
any other VELOC functions.

It is collective across the set of processes in the job. Within an MPI
application, it must be called collectively after ``MPI_Init()`` by all
processes within ``MPI_COMM_WORLD``.

It accomplishes the following:

#. Process configuration files and gather requirements
#. Setup internal variables, spawn/connect to back end VELOC processes,
   etc.
#. Rebuild or load checkpoint in cache (if available).

Finalize
^^^^^^^^

::

   int VELOC_Finalize (
      IN int cleanup
   )

ARGUMENTS
'''''''''

**FIXME** - **cleanup**:????

.. _description-1:

DESCRIPTION
'''''''''''

This function shuts down the VELOC library. No VELOC functions may be
called after calling this function.

It is collective across the set of processes in the job. Within an MPI
application, it must be called collectively before ``MPI_Finalize()`` by
all processes within ``MPI_COMM_WORLD``.

Memory Registration
~~~~~~~~~~~~~~~~~~~

Typically, applications register memory once after initializing the
VELOC library. Applications are free to register memory throughout the
run, including within restart and checkpoint phases. Applications can
also deregister memory regions that had been registered earlier in the
execution. All memory registration must be completed before using any of
the functions from \ `3.9 <#sec:fti>`__.

.. _memory-registration-1:

Memory Registration
^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Mem_protect (
     IN int id,
     IN void * ptr,
     IN size_t count,
     IN size_t base_size
   )
   
.. _arguments-2:

ARGUMENTS
'''''''''

-  **id**: This argument provides a application-defined integer label to
   refer to the memory region.
-  **ptr**: This is the pointer to the start of the memory region.
-  **count**: This refers to the number of consecutive elements in memory region.
-  **base_size**: This refers to the size of each element in memory region.
   

.. _description-3:

DESCRIPTION
'''''''''''

This function registers a memory region for checkpoint/restart. VELOC
internally associates the caller’s label ``id`` with the memory region
as defined by the pointer to the start of the region, the number of
elements, and the size of each elements in the region. The memory region
will be persisted during a checkpoint and recovered upon restart. If the
application specifies a value for ``id`` that has been used previously,
VELOC deregisters the previous region associated with that id and
records the pointer, count, and type provided in the new call. One may
specify ``count = 0`` to effectively deregister a memory region.

The function is local to each process. Each process may register an
arbitrary number of memory regions, and all ``id`` labels are unique to
each process.

Memory Deregistration
^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Mem_unprotect (
     IN int id
   )

.. _arguments-3:

ARGUMENTS
'''''''''

-  **id**: This argument provides a application-defined integer identifier to
   refer to the memory region.

.. _description-4:

DESCRIPTION
'''''''''''

This function deregisters a memory region for checkpoint/restart. Some
users may need to checkpoint different variables at various time steps
selectively. The memory deregistration interface allows users to remove
the registered memory at run time.

File Registration
~~~~~~~~~~~~~~~~~

Applications can only specify files within checkpoint and restart
phases. It is not valid for an application to refer to files outside of
such phases.

.. _file-registration-1:

File Registration
^^^^^^^^^^^^^^^^^

::

   int VELOC_Route_file (
     IN  char * ckpt_file_name,
   )
   
.. _arguments-4:

ARGUMENTS
'''''''''

-  **ckpt_file_name**: ***FIXME**

.. _description-5:

DESCRIPTION
'''''''''''

**FIXME:   Code says "obtain the full path for the file associated with the named checkpoint and version number", "can be used to manually read/write checkpointing data without registering memory regions." We need to modify the below text..


This function registers a file as belonging to a checkpoint/restart. The
application provides the name or path of the file it would open on the
parallel file system in the ``path`` argument. If this path is not
absolute, VELOC internally prepends the current working directory to
this value at the time of the call.

VELOC returns the actual path that the application must use when
opening/creating the file in ``newpath``. The caller must provide a
pointer to a buffer of at least ``VELOC_MAX_NAME`` characters to hold
this output value.

The function is local to each process. Each process may register an
arbitrary number of files.

Checkpoint Functions
~~~~~~~~~~~~~~~~~~~~

Open Checkpoint Phase
^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Checkpoint_begin (
     IN const char * name,
     int version
   )

.. _arguments-6:

ARGUMENTS
'''''''''

-  **name**: The name with which to label the checkpoint.
-  **version**: The version of the checkpoint, needs to increase with each checkpoint (e.g. iteration number)    

.. _description-7:

DESCRIPTION
'''''''''''

This function begins a checkpointing phase.

The caller labels the checkpoint with a name. The string in ``name``
should uniquely define the checkpoint. It must be no longer than
``VELOC_MAX_NAME`` characters, including the trailing NUL character.
This name is returned during a restart. It is also used by external
command line tools, so the name should generally be easy to type. The
same value must be provided for ``name`` on all processes.

It is collective across the set of processes in the job. Within an MPI
application, it must be called collectively by all processes within
``MPI_COMM_WORLD``.

Memory-based Checkpoint
^^^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Checkpoint_mem (
   )

.. _arguments-7:

ARGUMENTS
'''''''''

-  None

.. _description-8:

DESCRIPTION
'''''''''''

The function is local to each process. Any process that registers memory
must call this function.

This function writes all registered memory regions into the checkpoint. The function must be called between ``VELOC_Checkpoint_begin()`` and
``VELOC_Checkpoint_end()``.

Close Checkpoint Phase
^^^^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Checkpoint_end (
     IN int success
     **FIXME** Is there an IN version parameter as well?
   )

.. _arguments-8:

ARGUMENTS
'''''''''

-  **success**: Input flag indicating whether the calling process
   completed its checkpoint successfully.

.. _description-9:

DESCRIPTION
'''''''''''

This function marks end of a checkpoint phase.

**FIXME: Is the below para correct??**
Inform VELOC that all files for the current checkpoint are complete
(i.e., done writing and closed) and whether they are valid (i.e.,
written without error). A process must close all checkpoint files before
calling this function. A process should set ``success`` to 1 if either it
wrote its checkpoint data successfully or it had no data to checkpoint.
It should set ``success`` to 0 otherwise. VELOC will determine whether all
processes wrote their checkpoint files successfully.

It is collective across the set of processes in the job. Within an MPI
application, it must be called collectively by all processes within
``MPI_COMM_WORLD``.



Restart Functions
~~~~~~~~~~~~~~~~~

Indication of Restart
^^^^^^^^^^^^^^^^^^^^^

::

int VELOC_Restart_test (
   IN const char *name, 
   IN int version
 )


.. _arguments-9:

ARGUMENTS
'''''''''
- **name** : label of the checkpoint
- **version** : maximum version to look for

.. _description-10:

DESCRIPTION
'''''''''''

The paramater ``name'' is the label of the checkpoint. There parameter ``version'' is the maximum version to look for.

This routine determines whether an application can restart from a previous checkpoint.

It is collective across the set of processes in the job. Within an MPI
application, it must be called collectively by all processes within
``MPI_COMM_WORLD``. The same value is returned in ``flag`` on all
processes.

Open Restart Phase
^^^^^^^^^^^^^^^^^^

::

int VELOC_Restart_begin (
   IN const char *name, 
   IN int version
 )

.. _arguments-10:

ARGUMENTS
'''''''''

- **name** : label of the checkpoint
- **version** :  version to look for

.. _description-11:

DESCRIPTION
'''''''''''

This function marks start of the restart phase.

**FIXME** This call returns the name of the checkpoint in ``name``. The caller
must provide a pointer to a buffer of at least ``VELOC_MAX_NAME``
characters to hold this output value.

It is collective across the set of processes in the job. Within an MPI
application, it must be called collectively by all processes within
``MPI_COMM_WORLD``.

Memory-based Restart
^^^^^^^^^^^^^^^^^^^^
::

int VELOC_Restart_mem (
 )


.. _arguments-11:

ARGUMENTS
'''''''''

-  None

.. _description-12:

DESCRIPTION
'''''''''''

The function is local to each process. This routine reads registered memory regions from the checkpoint file

**FIXME** Each process must specify a unique name in ``file``. If ``file`` is not
an absolute path, the current working directory is prepended at the time
of the call. One should not call ``VELOC_Route_file()`` on this file.

Must be called between ``VELOC_Restart_begin()`` and
``VELOC_Restart_end()``.

Close Restart Phase
^^^^^^^^^^^^^^^^^^^

::

   int VELOC_Restart_end (
     IN int success
   )

.. _arguments-12:

ARGUMENTS
'''''''''

-  **sucess**: set to 1 if the state restore was successful, 0 otherwise.

.. _description-13:

DESCRIPTION
'''''''''''

This function marks the end of a restart phase.

A process should set ``success`` to 1 if either if state store is sucessful. It should set
``success`` to 0 otherwise. VELOC will determine whether all processes
read their checkpoint files successfully.

It is collective across the set of processes in the job. Within an MPI
application, it must be called collectively by all processes within
``MPI_COMM_WORLD``.

