cmake_minimum_required (VERSION 3.9)
project (VELOC)

# some global compiler flags
add_definitions(-O2 -g -Wall -D__BENCHMARK -D__ASSERT -D__INFO)
set(CMAKE_CXX_STANDARD 11)

# Now set up the libraries
# -----------------------------------------------------------------------------------

# set up PThreads
find_package(Threads REQUIRED)

# set up MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# -----------------------------------------------------------------------------------
include_directories(${VELOC_SOURCE_DIR})
add_subdirectory (src)

# -----------------------------------------------------------------------------------
enable_testing()
add_subdirectory (test)
