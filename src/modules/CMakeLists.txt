add_library (veloc-modules SHARED 
  module_manager.cpp
  client_watchdog.cpp transfer_module.cpp
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)
target_link_libraries(veloc-modules ${CMAKE_THREAD_LIBS_INIT})

# Install libraries
install (TARGETS veloc-modules
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
