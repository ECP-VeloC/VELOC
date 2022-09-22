add_library (veloc-client SHARED
  veloc.cpp
  client.cpp
  cache.cpp
  ${PROJECT_SOURCE_DIR}/src/backend/work_queue.cpp
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
  ${PROJECT_SOURCE_DIR}/include/veloc.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install (DIRECTORY
  ${PROJECT_SOURCE_DIR}/include/veloc
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
