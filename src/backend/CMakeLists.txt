add_executable (veloc-backend main.cpp work_queue.cpp)
target_link_libraries (veloc-backend veloc-modules ${COMM_QUEUE_LIBRARIES})

# Install executables
install (TARGETS veloc-backend
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
