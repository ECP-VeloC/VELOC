set(CMAKE_Fortran_FORNMAT "FIXED")
set(veloc_f90 interfaceVeloc.F90 velocf.c)
set_property(SOURCE ${veloc_f90} PROPERTY COMPILE_FLAGS "${MPI_Fortran_COMPILE_FLAGS} -fPIC")

if(VELOC_LINK_STATIC)
  add_library (veloc-fortran STATIC 
    ${veloc_f90}
    ${VELOC_SOURCE_DIR}/src/common/config.cpp
  )   
  set_target_properties(veloc-fortran PROPERTIES LINK_SEARCH_START_STATIC 1)
  set_target_properties(veloc-fortran PROPERTIES LINK_SEARCH_END_STATIC 1)
else()
  add_library (veloc-fortran SHARED
    ${veloc_f90}
    ${VELOC_SOURCE_DIR}/src/common/config.cpp
  )   
endif()

# Link the executable to the necessary libraries.
target_link_libraries (veloc-fortran ${MPI_Fortran_LIBRARIES} veloc-client)

# Install libraries
install (TARGETS veloc-fortran
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_Fortran_MODULE_DIRECTORY}/veloc.mod DESTINATION include)


