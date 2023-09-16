add_executable (veloc-inspect veloc-inspect.cpp)
target_link_libraries (veloc-inspect veloc-modules)

# Install executables
install (TARGETS veloc-inspect
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
