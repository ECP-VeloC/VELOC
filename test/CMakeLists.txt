# Comment as needed to reduce build time
add_executable (heatdis_original heatdis_original.c)
add_executable (heatdis_mem heatdis_mem.c)
add_executable (heatdis_file heatdis_file.c)
add_executable (heatdis_fault heatdis_fault.cpp)
if (SERIALIZATION_LIBRARIES)
  add_executable (cpp_test cpp_test.cpp)
endif()

# Link the executable to the necessary libraries.
target_link_libraries (heatdis_original PRIVATE m MPI::MPI_C)
target_link_libraries (heatdis_mem PRIVATE m veloc::client)
target_link_libraries (heatdis_file PRIVATE m veloc::client)
target_link_libraries (heatdis_fault PRIVATE m veloc::client)
if (SERIALIZATION_LIBRARIES)
  target_link_libraries (cpp_test PRIVATE veloc::client ${SERIALIZATION_LIBRARIES})
endif()

# Configure test paths
set(CMAKE_TEST_SCRATCH /tmp/scratch CACHE PATH "Scratch path for CMake test")
set(CMAKE_TEST_PERSISTENT /tmp/persistent CACHE PATH "Persistent path for CMake test")
set(CMAKE_TEST_META /tmp/meta CACHE PATH "Metadata path for CMake test")
configure_file(heatdis.in heatdis.cfg @ONLY)
configure_file(test-async.in test-async.sh @ONLY)
configure_file(test-cpp.in test-cpp.sh @ONLY)

# Add test
add_test(cpp test-cpp.sh)
add_test(async test-async.sh)
