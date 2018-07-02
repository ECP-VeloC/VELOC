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
    
If you want to experiment with the latest development version, you can directly check out
the master branch. This is helpful to stay up-to-date with the latest features but will likely
have issues:

::

    git clone https://github.com/ECP-VeloC/veloc.git

Install VeloC
~~~~~~~~~~~~~

VeloC has an automated installation process based on Python, which depends on several standard libraries.
These standard libraries may not be present on your system. If that is the case, you need to bootstrap the installation
process first as follows: 

::

   $bootstrap.sh

Once the bootstrapping has finished, the ``auto-install.py`` script will build and install VeloC and all it dependencies.
The script can be edited to modify certain compiler options if needed. Common compiler options needed for some machines
(e.g. Cray) are included as comments. After editing the script, run it as follows:

::

   $auto-install.py <destination_path>
   
Note that it may be possible that your Python installation will not detect the libraries installed by the bootstrapping 
automatically. In this case, locate the installed libraries and tell Python about them as follows:

::

    $setenv PYTHONPATH ~/.local/lib/python3.6/site-packages

If the installation process was successful, the VeloC client library (and its dependencies) are installed under
``<destination_path>/lib``. The ``veloc.h`` header needed by the application developers to call the VeloC API is 
installed under ``<destination_path>/include``. The active backend needed to run VeloC in asynchronous mode can be found in
the source code repository: ``src/backend/veloc-backend``.

Configure VeloC
~~~~~~~~~~~~~~~

VeloC uses a INI-style configuration with the following options:

::

   scratch = <local_path>
   persistent = <shared_path>
   mode = <sync|async>
   ec_interval = <seconds> (default: 0)
   persistent_interval = <seconds> (default: 0)
   max_versions = <int> (default: 0)
   axl_type = AXL_XFER_SYNC (default: N/A)

The first three options are mandatory and specify where VeloC can save local checkpoints and redundancy information 
for collaborative resilience strategies (currently set to XOR encoding). All other options are not 
mandatory and have a default. Every time the application issues a checkpoint request, the local checkpoints are saved 
in the scratch path of the node. Erasure coding is active by default and is applied to all checkpoint versions. To specify
a minimum amount of time that needs to pass between checkpoints protected by erasure coding, ``ec_interval`` can be set to 
a number of seconds. If ``ec_interval`` is negative, erasure coding is deactivated. Similarly, flushing of the local 
checkpoints to the parallel file system is active by default and can be controlled using ``persistent_interval``. To
preserve space, users can specify ``max_versions`` to instruct VeloC to keep only the latest N checkpoint versions. This
applies to the scratch and persistent level individually. Finally, the user can specify whether to use a built-in POSIX
file transfer routine to flush the files to a parallel file system or to use the AXL library for optimized flushes that can
take advantage of additional hardware to accelerate I/O (such as burst buffers).

.. _ch:velocrun:

Running VeloC
-------------

VeloC can be run in either synchronous mode (all resilience strategies are embedded in the client library and run directly 
in the application processes in blocking fashion) or asynchronous mode (the resilience strategies run in the active backend
in the background). 

To use VeloC in synchronous mode, the application simply needs to be run as any normal MPI job. To run VeloC in 
asynchronous mode, each node needs to run an active backend instance:

::

   mpirun -np N --map-by ppr:1:node <path>/veloc-backend <config_file>
   
After the active backends are up and running, the application can run as a normal MPI job. Each application process will 
then connect to the local backend present on the node where it is running.

Examples
~~~~~~~~

VeloC comes with a series of examples in the ``test`` subdirectory that can be used to test the setup. To run these 
examples (in either synchronous or asynchronous mode), edit the sample configuration file ``heatdis.cfg`` and then run 
the application as follows (run the active backend first as mentioned above if in async mode):

::

   mpirun -np N test/heatdis_mem <mem_per_process> <config_file>

Job Scheduler Interaction
-------------------------

HPC machines are typically configured to run the user applications as batch jobs. Therefore, the user needs to make sure
that the job scheduler is not configured to kill the entire job when a node fails. Assuming the job scheduler is configured
correctly, the user needs to write a script as follows:

::

    reserve N+K nodes (to survive a maximum of K total failures over the entire application runtime) 
    do
        run the active backend if not alive (on the surviving nodes)
        run the application (on the surviving nodes)
    while (failure detected) // e.g, exit code of the application


