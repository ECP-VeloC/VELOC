# VELOC: VEry-Low Overhead Checkpointing System

VELOC is a multi-level checkpoint/restart runtime that delivers 
high performance and scalability for complex heterogeneous storage 
hierarchies without sacrificing ease of use and flexibility.

It is primarily used as a fault-tolerance tool for tightly coupled
HPC applications running on supercomputing infrastructure but is
essential in many other use cases: suspend-resume, migration, 
debugging, roll-back and explore alternative directions. 

VELOC is a collaboration between Argonne National Laboratory and 
Lawrence Livermore National Laboratory as part of the Exascale 
Computing Project.

## Documentation

The documentation of VeloC is available here: http://veloc.rtfd.io

It includes a quick start guide as well that covers the basics needed
to use VELOC on your system.

## Availability

VELOC is part of the Extreme-scale Scientific Software Stack 
([E4S](https://e4s-project.github.io)). Chances are it is already 
pre-installed on large supercomputing systems (e.g., as 
a [module](https://modules.readthedocs.io/en/latest)). 

Furthermore, it is integrated with [Spack](https://github.com/spack/spack) 
package manager.

## Bindings

VELOC supports checkpointing of complex data structures in Python through
a straightforward API that hides the details of serialization using the
pickle module. Furthermore, it seamlessly integrates with popular C++ 
serialization libraries such as Cereal and Bitsery.

## Contacts

In case of questions and comments or help, please contact the VeloC team at 
veloc-users@lists.mcs.anl.gov

## Release

Copyright (c) 2018-present, UChicago Argonne LLC, operator of Argonne National Laboratory <br>
Copyright (c) 2018-present, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.

For release details and restrictions, please read the [LICENSE](https://github.com/ECP-VeloC/VELOC/blob/master/LICENSE) 
and [NOTICE](https://github.com/ECP-VeloC/VELOC/blob/master/NOTICE) files.

`LLNL-CODE-751725` `OCEC-18-060`
