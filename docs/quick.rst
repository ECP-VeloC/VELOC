Quick Start Guide
=================

This guide is for the impatient who wants to run and test a minimal
VeloC environment on a single node to get a first impression of how
it works. It can be achieved in three-steps:

Download and Install
--------------------

::

    $git clone -b 'veloc-x.y' --depth 1 https://github.com/ECP-VeloC/veloc.git <source_dir>
    $cd <source_dir>
    $./bootstrap.sh
    $./auto-install.py <install_dir>

Note: replace ``x.y`` with the latest stable version available on github.

Configure
---------

Create and swich to the temporary working directory ``/tmp/work``.
Open ``test.cfg`` and add the following contents:

::

    scratch = /tmp/scratch
    persistent = /tmp/persistent
    mode = async

Run VeloC
---------

Open a terminal and run the heatdis example application using two MPI ranks:

::

    $export LD_LIBRARY_PATH=<install_dir>/lib
    $export PATH=<install_dir>/bin:$PATH
    $mpirun -np 2 <source_dir>/build/test/heatdis_mem 256 test.cfg

If the run was successful, the scratch and persistent directories will be populated 
with checkpoint files of the form heatdis-x-y.dat, where is is the rank and y is
the iteration number. Also, a log file should have been created where the active
backend (the VELOC service running on each node which responsible for checkpoint management)
displays important information: ``/dev/shm/veloc-backend-<hostname>-<uid>.log``.

Now, delete one of the checkpoints with the highest version
from both directories, then run the same command again. The application should
restart from the checkpoint version immediately preceding the highest version and
continue running to completion. After the second run, the directories should be
populated with the same checkpoint files as in the case of the previous run.

Congratulations, this concludes the quick start guide!
