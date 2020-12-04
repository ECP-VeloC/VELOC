add_library (veloc-modules SHARED
  module_manager.cpp
  client_watchdog.cpp daos_module.cpp chksum_module.cpp versioning_module.cpp # simple modules
  client_aggregator.cpp ec_module.cpp # aggregation modules
  ${PROJECT_SOURCE_DIR}/src/common/config.cpp
  ${PROJECT_SOURCE_DIR}/src/common/file_util.cpp
)
target_link_libraries(veloc-modules ${MPI_CXX_LIBRARIES} ${ER_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY} ${DAOS_LIBRARIES})

# Install libraries
install (TARGETS veloc-modules
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
