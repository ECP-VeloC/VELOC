cmake_minimum_required (VERSION 3.9)
project (VELOC)

# some global compiler flags
add_definitions(-O0 -g -Wall -D__BENCHMARK -D__ASSERT -D__INFO)

# Now set up the libraries
# -----------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${VELOC_SOURCE_DIR}/cmake")

# set up boost
set(Boost_FIND_REQUIRED True)
find_package(Boost 1.60)
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

# -----------------------------------------------------------------------------------

include_directories(${VELOC_SOURCE_DIR})
add_subdirectory (src)
add_subdirectory (test)
