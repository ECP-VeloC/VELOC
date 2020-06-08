# Comment as needed to reduce build time
add_executable (heatdis_original heatdis_original.c)
add_executable (heatdis_mem heatdis_mem.c)
add_executable (heatdis_file heatdis_file.c)
add_executable (heatdis_fault heatdis_fault.cpp)

# Link the executable to the necessary libraries.
target_link_libraries (heatdis_original m ${MPI_C_LIBRARIES})
target_link_libraries (heatdis_mem m veloc-client)
target_link_libraries (heatdis_file m veloc-client)
target_link_libraries (heatdis_fault m veloc-client)

configure_file(test-async.in test-async.sh @ONLY)
add_test(async test-async.sh)
