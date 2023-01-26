add_library (veloc-modules SHARED
  module_manager.cpp
  # simple modules
  client_watchdog.cpp transfer_module.cpp chksum_module.cpp versioning_module.cpp
  # aggregation modules
  client_aggregator.cpp ec_module.cpp
  # storage modules
  ${PROJECT_SOURCE_DIR}/src/storage/storage_module.cpp
  ${PROJECT_SOURCE_DIR}/src/storage/posix_module.cpp
  ${PROJECT_SOURCE_DIR}/src/storage/posix_agg_module.cpp
  # common code
  ${PROJECT_SOURCE_DIR}/src/common/command.cpp
  ${PROJECT_SOURCE_DIR}/src/common/config.cpp
  ${PROJECT_SOURCE_DIR}/src/common/file_util.cpp)

add_library (veloc::modules ALIAS veloc-modules)

target_link_libraries (veloc-modules PUBLIC MPI::MPI_CXX er::er OpenSSL::Crypto)

if (axl_FOUND)
  target_sources (veloc-modules PRIVATE  ${PROJECT_SOURCE_DIR}/src/storage/axl_module.cpp)
  target_link_libraries (veloc-modules PUBLIC axl::axl)
endif()
if (DAOS_FOUND)
  target_sources (veloc-modules PRIVATE  ${PROJECT_SOURCE_DIR}/src/storage/daos_module.cpp)
  target_link_libraries (veloc-modules PUBLIC ${DAOS_LIBRARIES})
endif()

target_include_directories (veloc-modules PUBLIC
  $<BUILD_INTERFACE:${VELOC_SOURCE_DIR}>
  $<BUILD_INTERFACE:${VELOC_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${VELOC_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

set_target_properties (veloc-modules PROPERTIES EXPORT_NAME modules)

# Install libraries
install (TARGETS veloc-modules EXPORT velocTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
