add_executable (veloc-backend
  main.cpp 
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)

# Enable static linking
if(VELOC_LINK_STATIC)
  set_target_properties(veloc-backend PROPERTIES LINK_SEARCH_START_STATIC 1)
  set_target_properties(veloc-backend PROPERTIES LINK_SEARCH_END_STATIC 1)
endif(VELOC_LINK_STATIC)

# Link the executable to the necessary libraries.
target_link_libraries (veloc-backend veloc-modules ${MPI_CXX_LIBRARIES} rt)

# Install executables
install (TARGETS veloc-backend
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
