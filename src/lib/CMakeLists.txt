add_library (veloc-client SHARED
  veloc.cpp
  client.cpp
)

# Link the executable to the necessary libraries.
target_link_libraries (veloc-client veloc-modules ${COMM_QUEUE_LIBRARIES})

# Install libraries
install (TARGETS veloc-client
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install (FILES
  ${PROJECT_SOURCE_DIR}/include/veloc.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
