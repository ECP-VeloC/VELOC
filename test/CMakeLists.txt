# Comment as needed to reduce build time
add_executable (heatdis heatdis.c)

# Link the executable to the necessary libraries.
target_link_libraries (heatdis ${MPI_C_LIBRARIES} m veloc)
