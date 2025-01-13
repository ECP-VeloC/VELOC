Overview
--------

VeloC is a multi-level checkpoint-restart runtime for HPC supercomputing infrastructures and large-scale data centers.
It aims to delivers high performance and scalability for complex heterogeneous storage hierarchies without sacrificing ease
of use and flexibility.

Checkpoint-Restart is primarily used as a fault-tolerance mechanism for tightly coupled HPC applications but
is essential in many other use cases: suspend-resume, migration, debugging. Some applications need to save their state and
revisit such previously saved states as part of the execution model (e.g. adjoint computations), which can be also addressed
using the checkpoint-restart pattern.

VeloC is a collaboration between Argonne National Laboratory and Lawrence Livermore National Laboratory as part of the
Exascale Computing Project.

Citations
---------

If you are using VELOC for your research projects, please cite the following papers in your reports and/or publications:

[1] Nicolae, B., Moody, A., Kosinovsky, G., Mohror, K. and Cappello, F. 2021. `VELOC: VEry Low Overhead Checkpointing in the Age of Exascale <https://arxiv.org/pdf/2103.02131.pdf>`. In SuperCheck’21: The First International Symposium on Checkpointing for Supercomputing, 2021

[2] Nicolae, B., Moody, A., Gonsiorowski, E., Mohror, K. and Cappello, F. `VeloC: Towards High Performance Adaptive Asynchronous Checkpointing at Large Scale <https://doi.org/10.1109/IPDPS.2019.00099>`. In IPDPS’19: The 2019 IEEE International Parallel and Distributed Processing Symposium, pp. 911–920, 2019

Software
--------

The VeloC software is available here: https://github.com/ecp-veloc/veloc

Team
----

(PI) Franck Cappello
Argonne National Laboratory

(Co-PI) Kathryn Mohror
Lawrence Livermore National Laboratory

Bogdan Nicolae
Argonne National Laboratory

Rinku Gupta
Argonne National Laboratory

Sheng Di
Argonne National Laboratory

Adam Moody
Lawrence Livermore National Laboratory

Elsa Gonsiorowski
Lawrence Livermore National Laboratory

Gregory Becker
Lawrence Livermore National Laboratory

Greg Kosinovsky
Lawrence Livermore National Laboratory

Contact Us
----------

In case of questions and comments or help, please contact the VeloC team at veloc-users@lists.mcs.anl.gov
