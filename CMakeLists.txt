cmake_minimum_required (VERSION 3.9)
project (VELOC)

# set the root prefix of the library dependencies
set(EXTERNAL_ROOT $ENV{HOME}/deploy)

# some global compiler flags
add_definitions(-O2 -Wall -D__BENCHMARK -D__ASSERT -D__INFO)

# Now set up the libraries
# -----------------------------------------------------------------------------------

# the elegant way to set up boost
set(BOOST_ROOT ${EXTERNAL_ROOT})
set(Boost_FIND_REQUIRED True)
find_package(Boost 1.60 COMPONENTS filesystem system serialization mpi)
include_directories(${Boost_INCLUDE_DIR})

# set up OpenSSL
find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR})

# set up Threads
find_package(Threads)

# set up MPI
find_package(MPI)
include_directories(${MPI_INCLUDE_PATH})

# -----------------------------------------------------------------------------------

include_directories(${VELOC_SOURCE_DIR})
add_subdirectory (src)
add_subdirectory (test)
