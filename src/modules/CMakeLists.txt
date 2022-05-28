set(VELOC_MODULES_SRC
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
  ${PROJECT_SOURCE_DIR}/src/common/file_util.cpp
)

set(VELOC_MODULES_LIB ${MPI_CXX_LIBRARIES} ${ER_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY})

if (AXL_FOUND)
  set(VELOC_MODULES_SRC ${VELOC_MODULES_SRC} ${PROJECT_SOURCE_DIR}/src/storage/axl_module.cpp)
  set(VELOC_MODULES_LIB ${VELOC_MODULES_LIB} ${AXL_LIBRARIES})
endif()
if (DAOS_FOUND)
  set(VELOC_MODULES_SRC ${VELOC_MODULES_SRC} ${PROJECT_SOURCE_DIR}/src/storage/daos_module.cpp)
  set(VELOC_MODULES_LIB ${VELOC_MODULES_LIB} ${DAOS_LIBRARIES})
endif()

add_library (veloc-modules SHARED ${VELOC_MODULES_SRC})
target_link_libraries(veloc-modules ${VELOC_MODULES_LIB})

# Install libraries
install (TARGETS veloc-modules
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
