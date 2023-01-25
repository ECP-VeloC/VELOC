add_library (veloc-client SHARED
  veloc.cpp
  client.cpp
  posix_cache.cpp
  ${PROJECT_SOURCE_DIR}/src/backend/work_queue.cpp
)

add_library (veloc::client ALIAS veloc-client)

# Link the executable to the necessary libraries.
target_include_directories (veloc-client PUBLIC $<BUILD_INTERFACE:${VELOC_SOURCE_DIR}> $<INSTALL_INTERFACE:include>)
target_link_libraries (veloc-client PUBLIC veloc::modules ${COMM_QUEUE_LIBRARIES})
set_target_properties (veloc-client PROPERTIES EXPORT_NAME client)

# Install libraries
install (TARGETS veloc-client EXPORT velocTargets
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
