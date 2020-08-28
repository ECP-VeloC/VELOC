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

News
----

We are interested in learning about the checkpoint/restart needs of HPC applications. Please feel free to complete the following survey multiple times if you are familiar with more than one application: https://goo.gl/forms/xBxv4pmv7MH0z5582

Citations
---------

If you are using VELOC for your research projects, please cite the following paper in your reports and/or publications:

Nicolae, B., Moody, A., Gonsiorowski, E., Mohror, K. and Cappello, F. 2019. VeloC: Towards High Performance Adaptive Asynchronous Checkpointing at Large Scale. IPDPS’19: The 2019 IEEE International Parallel and Distributed Processing Symposium, pp. 911–920, Rio de Janeiro, Brazil, 2019
(available here: https://doi.org/10.1109/IPDPS.2019.00099)

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

Contact Us
----------

In case of questions and comments or help, please contact the VeloC team at veloc-users@lists.mcs.anl.gov
