# Comment as needed to reduce build time
add_executable (heatdis heatdis.c)
add_executable (heatdis_file heatdis_file.c)

# Link the executable to the necessary libraries.
target_link_libraries (heatdis ${MPI_C_LIBRARIES} m veloc-client)
target_link_libraries (heatdis_file ${MPI_C_LIBRARIES} m veloc-client)
