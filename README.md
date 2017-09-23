# VELOC: VEry-Low Overhead Checkpointing system

VELOC software version 0.5

The VELOC project aims to provide a framework offering a checkpoint/restart interface and providing transparently the benefit of multilevel checkpointing to exascale applications. VELOC is a collaboration between Argonne National Laboratory and Lawrence Livermore National Laboratory as part of the Exascale Computing Project.


## VELOC API

This implementation of VELOC software is based on VELOC API (version 0.4 currently).The VELOC API can be obtained from the https://xgitlab.cels.anl.gov/ecp-veloc/Veloc-Documentation.git

## Project Setup

Current VELOC software (veloc-0.5) implements the VELOC API in the VELOC client layer library and for now uses SCR as the VELOC backend. The software has scripts in place to download and builds the current version SCR.


## Quickstart

Note that users need to use CMake to build the VELOC library. Also note that this example uses out-of-source builds, placing the `build` and `install` directories at the same level as the `veloc` directory.

The following commands should download VELOC (along with the SCR backend for now), build and install it and create test examples.

```shell
mkdir build
mkdir install
git clone git@xgitlab.cels.anl.gov:ecp-veloc/veloc.git

cd build
cmake -DCMAKE_INSTALL_PREFIX=../install -DBUILD_PDSH=ON ../veloc
make -k
make test
```

### SCR Build Error
Note that currently, SCR requires the PDSH library.
To automatically download and build PDSH as part of the CMake build, use the option `-DBUILD_PDSH=ON` when configuring CMake.

## Testing

We currently test three ways to interact with the VeloC API:

- Memory interface
- File-based interface
- A hybrid interface mixing memory-interface and file-based interface paradigms

The tests can be run using `make test` or, for more verbose output, using `make check`.


## Contacts
In case of questions and comments or help, please contact the VELOC team at ecp-veloc@lists.mcs.anl.gov




