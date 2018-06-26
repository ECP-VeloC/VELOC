Overview
--------
The Mean Time Between Node Failure (MTBNF) of the DOE CORAL systems and future exascale systems imposes a fault tolerance mechanism to guarantee that executions complete successfully even in case of node failures. Currently, the majority of HPC applications use ad hoc checkpoint/restart for fault tolerance. The addition of nonvolatile memory and burst buffers in systems will require nontrivial code modifications to handle the diversity of architectures and to actually realize benefit from these new levels of storage. Dozens of very complex applications will need significant modifications. Moreover, without specific optimizations for fast restart from checkpoints stored on nonvolatile memory, the execution will potentially experience very long delays in retrieving the checkpoint on the file system. The VeloC project will address these challenges by providing a framework offering a checkpoint/restart interface and providing transparently the benefit of multilevel checkpointing to exascale applications.

VeloC will have three major impacts. It will: (1) optimize checkpoint/restart performance for applications and workflows (2) increase programmer productivity by dramatically reducing the difficulty of handling varied and complex storage architectures and the need for performance/reliability optimizations. (3) benefit a wide community.

Objectives

• Provide a single API for data structure oriented and file oriented checkpoint/restart

• Provide an active back-end performing the checkpoint movements concurrently and asynchronously to the application execution

• Optimize multi-level checkpointing for the available CORAL and ECP relevant systems with deep/complex storage hierarchies: local memory, NVM, local burst-buffers, remote burst buffers, parallel file systems.

• Integrate VeloC in I/O libraries (HDF5, ADIOS, PnetCDF), advanced batch scheduler, Vendor data movement software


Software
--------
This implementation of VELOC software is based on VELOC API (version 0.4 currently).The VELOC API can be obtained from the https://xgitlab.cels.anl.gov/ecp-veloc/Veloc-Documentation.git

The VeloC software is available at https://github.com/ecp-veloc/veloc

Team
----
(PI) Franck Cappello
Argonne National Laboratory

(Co-PI) Kathryn Mohror
Lawrence Livermore National Laboratory

Sheng Di 
Argonne National Laboratory

Rinku Gupta
Argonne National Laboratory

Bogdan Nicolae
Argonne National Laboratory

Gregory Becker
Lawrence Livermore National Laboratory

Elsa Gonsiorowski
Lawrence Livermore National Laboratory

Adam Moody
Lawrence Livermore National Laboratory


Publications & Presentations
----------------------------


Contact Us
----------
In case of questions and comments or help, please contact the VELOC team at ecp-veloc@lists.mcs.anl.gov
