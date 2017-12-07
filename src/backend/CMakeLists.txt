# Comment as needed to reduce build time
add_executable (backend main.cpp topology.cpp module_manager.cpp)

# Link the executable to the necessary libraries.
target_link_libraries (backend ${MPI_CXX_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} rt)

# Install executables
install (TARGETS backend
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
