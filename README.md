# VELOC: VEry-Low Overhead Checkpointing system

VELOC is a collaboration between Argonne National Laboratory and Lawrence Livermore National Laboratory as part of the Exascale Computing Project.
It is under development by the [FTI](https://github.com/leobago/fti) and [SCR](https://github.com/llnl/scr) development teams.

## Example Project

This simple example downloads and builds the current version SCR.
FTI functionality is implemented on top of SCR directly in the `veloc.c` file.
The current VELOC API is defined in the `veloc.h` header file.

## Quickstart

Use CMake to build the VELOC library.
This will download the latest versions of SCR.

Note: This example uses out-of-source builds, placing the `build` and `install` directories at the same level as the `veloc` directory.

```shell
mkdir build
mkdir install
git clone git@xgitlab.cels.anl.gov:ecp-veloc/veloc.git

cd build
cmake -DCMAKE_INSTALL_PREFIX=../install ../veloc
make -k
make test
```

### SCR Build Error

Currently, SCR requires the PDSH library.
To automatically download and build PDSH as part of the CMake build, use the option `-DBUILD_PDSH=ON` when configuring CMake.
