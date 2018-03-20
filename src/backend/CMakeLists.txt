add_executable (veloc-backend
  main.cpp 
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)

# Link the executable to the necessary libraries.
target_link_libraries (veloc-backend veloc-modules ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} rt)

# Install executables
install (TARGETS veloc-backend
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
