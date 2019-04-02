cmake_minimum_required (VERSION 3.9)
project (VELOC)

# some global compiler flags
add_definitions(-O2 -g -Wall -D__BENCHMARK -D__ASSERT -D__INFO)
set(CMAKE_CXX_STANDARD 11)

# Now set up the libraries
# -----------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${VELOC_SOURCE_DIR}/cmake")

# Configuration Options

SET(VELOC_RESOURCE_MANAGER "LSF" CACHE STRING "Resource Manager for CLI (SLURM LSF NONE)")
SET_PROPERTY(CACHE VELOC_RESOURCE_MANAGER PROPERTY STRINGS SLURM LSF NONE)

## PDSH
FIND_PACKAGE(PDSH REQUIRED)

# set up boost
set(Boost_FIND_REQUIRED True)
find_package(Boost 1.53)
include_directories(${Boost_INCLUDE_DIR})

# set up PThreads
find_package(Threads REQUIRED)

# set up MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# find dependencies
find_package(AXL REQUIRED)
include_directories(${AXL_INCLUDE_DIRS})

find_package(ER REQUIRED)
include_directories(${ER_INCLUDE_DIRS})

INCLUDE(GNUInstallDirs)
## Use X_ variable names for CLI scripts
## could use CMAKE_INSTALL_FULL_ names instead
SET(X_BINDIR ${CMAKE_INSTALL_FULL_BINDIR} CACHE INTERNAL "bin")
SET(X_DATADIR ${CMAKE_INSTALL_FULL_DATADIR} CACHE INTERNAL "share")
SET(X_INCLUDEDIR ${CMAKE_INSTALL_FULL_INCLUDEDIR} CACHE INTERNAL "include")
SET(X_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR} CACHE INTERNAL "lib")

# -----------------------------------------------------------------------------------
add_subdirectory(scripts)

# -----------------------------------------------------------------------------------
include_directories(${VELOC_SOURCE_DIR})
add_subdirectory (src)

# -----------------------------------------------------------------------------------
enable_testing()
add_subdirectory (test)
