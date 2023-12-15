# Comment as needed to reduce build time
add_executable (heatdis_original heatdis_original.c)
add_executable (heatdis_mem heatdis_mem.c)
add_executable (heatdis_file heatdis_file.c)
add_executable (heatdis_fault heatdis_fault.cpp)

# Link the executable to the necessary libraries.
target_link_libraries (heatdis_original PRIVATE m MPI::MPI_C)
target_link_libraries (heatdis_mem PRIVATE m veloc::client)
target_link_libraries (heatdis_file PRIVATE m veloc::client)
target_link_libraries (heatdis_fault PRIVATE m veloc::client)

# Configure test paths
set(CMAKE_TEST_SCRATCH /tmp/scratch CACHE PATH "Scratch path for CMake test")
set(CMAKE_TEST_PERSISTENT /tmp/persistent CACHE PATH "Persistent path for CMake test")
set(CMAKE_TEST_META /tmp/meta CACHE PATH "Metadata path for CMake test")
configure_file(test-async.in test-async.sh @ONLY)
configure_file(heatdis.in heatdis.cfg @ONLY)

# Add test
add_test(async test-async.sh)
