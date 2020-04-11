add_library (veloc-modules SHARED 
  module_manager.cpp
  client_watchdog.cpp transfer_module.cpp chksum_module.cpp
  client_aggregator.cpp ec_module.cpp
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
  ${VELOC_SOURCE_DIR}/src/common/file_util.cpp
)
target_link_libraries(veloc-modules ${ER_LIBRARIES} ${AXL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_CRYPTO_LIBRARY})

# Install libraries
install (TARGETS veloc-modules
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
