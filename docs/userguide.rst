User Guide
===========

This documentation is intended for users who need to run applications that make use of VeloC for 
checkpoint/restart.

.. _ch:velocsetup:

Setup
-----

Due to the large number of software and hardware configurations where VeloC
can run, it must be built from source. Once built and installed, VeloC needs
to be configured using a configuration file. These aspects are detailed below:

Download VeloC
~~~~~~~~~~~~~~

The source code of VeloC is publicly available on ``github``. To download it,
look for the latest stable version x.y, which should appear under the 
'Releases/Tags' tab as 'veloc-x.y'. Then, use the following command:

::

    git clone -b 'veloc-x.y' --single-branch --depth 1 https://github.com/ECP-VeloC/veloc.git
    
If you want to experiment with the latest development version, you can directly check out the main branch.
This is helpful to stay up-to-date with the latest features. The main branch is extensively tested and
can be considered stable for most practical purposes.

::
    
    git clone --single-branch --depth 1 https://github.com/ECP-VeloC/veloc.git

Install VeloC
~~~~~~~~~~~~~

VeloC has an automated installation process based on Python, which depends on several standard libraries.
These standard libraries may not be present on your system. If that is the case, you need to bootstrap the installation
process first as follows: 

::

   $./bootstrap.sh

Once the bootstrapping has finished, the ``auto-install.py`` script will build and install VeloC and all it dependencies.
The installation can be fine-tuned with several options, which can be listed by supplying the "--help" switch. Notably, you
can control the installation directory, communication protocol between the client and the backend, whether to use pre-installed
libraries, etc. The script can be edited to modify certain compiler options if needed. Common compiler options needed for some 
machines (e.g. Cray) are included as comments. After editing the script, run it as follows:

::

   $./auto-install.py <install_dir>
   
Note that it may be possible that your Python installation will not detect the libraries installed by the bootstrapping 
automatically. In this case, the ``auto-install.pu`` script will fail. Locate the installed libraries and tell Python about them as follows:

::

    $setenv PYTHONPATH ~/.local/lib/python3.6/site-packages

If the installation process was successful, the VeloC client library (and its dependencies) are installed under
``<install_dir>/lib``. The ``veloc.h`` header needed by the application developers to call the VeloC API is 
installed under ``<install_dir>/include``. The active backend needed to run VeloC in asynchronous mode can be found in
``<install_dir/bin/veloc-backend``. The examples can be found in ``<source_dir>/src/test``, while the corresponding compiled executables 
are here: ``<source_dir>/build/test``.

Configure VeloC
~~~~~~~~~~~~~~~

VeloC uses a INI-style configuration with the following mandatory fields:

:: 

  scratch = <path> (node-local path where VELOC can save temporary checkpoints that live for the duration of the reservation)
  persistent = <path> (persistent path where VELOC can save durable checkpoints that live indefinitely)
  
In addition, the following optional fields are available:

::

  persistent_interval = <int> (seconds between consecutive persistent checkpoints, default: 0 - perform all)
  ec_interval = <int> (seconds between consecutive EC checkpoints, default: 0 - perform all)
  watchdog_interval = <int> (seconds between consecutive checks of client processes: default: 0 - don't check)
  max_versions = <int> (number of previous checkpoints to keep on persistent, default: 0 - keep all)
  scratch_versions = <int> (number of previous checkpoints to keep on scratch, default: 0 - keep all)
  failure_domain = <string> (failure domain used for smart distribution of erasure codes, default: <hostname>)
  axl_type = <string> (AXL read/write strategy to/from the persistent path, default: <empty> - deactivate AXL)
  chksum = <boolean> (activates checksum calculationa and verification for checkpoints, default: false)
  meta = <path> (persistent path where VELOC will save checksumming information)
  
Both the persisten and ec interval can be set to -1, which fully deactivates that feature. This is preferred to setting a high number
(which also works but is less readable and has slightly higher overhead because VeloC will need to do extra checks). If you leave
scratch_versions to the default value, you must ensure the scratch mount point will run out of space. VELOC will not automatically
delete checkpoints when space is low. If space is a concern, set scratch_versions accordingly. Similar observations apply for
the persistent mount point, for which the corresponding option is max_versions. Finally, you can specify whether to use a built-in 
POSIX file transfer routine to flush the files to a parallel file system or to use the AXL library for optimized flushes that can
take advantage of additional hardware to accelerate I/O (such as burst buffers). If the use of AXL is desired, you need to specify
as ``axl_type`` as per the AXL documentation (which is part of VELOC). Note that VELOC uses a separate ``meta`` path for checksumming
information, instead of writing checksumming information directly into the checkpoints. Thus, it is perfectly valid to save checksumming 
information during checkpointing but then delete or ignore it later on restart (in which case the ``meta`` option must be omitted).

.. _ch:velocrun:

Execution
---------

VeloC can be run in either synchronous mode (all resilience strategies are embedded in the client library and run directly 
in the application processes in blocking fashion) or asynchronous mode (the resilience strategies run in a separate process
called the active backend in asynchronous mode in the background). 

To use VeloC in synchronous mode, the application simply needs to be run as any normal MPI job. To run VeloC in 
asynchronous mode, you need to make sure the ``veloc-backend`` executable can either be found in the ``$PATH`` or
``$VELOC_BIN`` environment variable. This is true for every node running the MPI ranks of your application (thus,
``veloc-backend`` should be accessible through a shared mount point). By default, ``veloc-backend`` will create 
the following log file on each node: ``/dev/shm/veloc-backend-<host_name>-<uid>.log``. The log file contains 
important information (error messages, time to flush to PFS, etc.) that you may want to collect and inspect while/after
running your application. In this case, you can control where the log files are saved using ``$VELOC_LOG`` environment
variable (e.g., a shared directory).

Examples
~~~~~~~~

VeloC comes with a series of examples in the ``test`` subdirectory that can be used to test the setup. To run these 
examples (in either synchronous or asynchronous mode), edit the sample configuration file ``heatdis.cfg`` and then run 
the application as follows (run the active backend first as mentioned above if in async mode):

::

   mpirun -np N <source_dir>/build/test/heatdis_mem <mem_per_process> <config_file>

Batch Jobs
----------

HPC machines are typically configured to run the user applications as batch jobs. Therefore, the user needs to make sure
that the job scheduler is not configured to kill the entire job when a node fails. Assuming the job scheduler is configured
correctly, the user needs to write a script as follows:

::

    reserve N+K nodes (to survive a maximum of K total failures over the entire application runtime) 
    do
        run the application (on the surviving nodes)
    while (failure detected) // e.g, exit code of the application
