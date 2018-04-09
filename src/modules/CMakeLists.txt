add_library (veloc-modules SHARED 
  module_manager.cpp
  client_watchdog.cpp transfer_module.cpp
  client_aggregator.cpp ec_module.cpp
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)
target_link_libraries(veloc-modules ${ER_LIBRARIES})

# Compile and link with AXL if found
find_package(AXL)
if(${AXL_FOUND})
    target_compile_definitions(veloc-modules PRIVATE AXL_FOUND=$<BOOL:${AXL_FOUND}>)
    target_include_directories(veloc-modules PRIVATE ${AXL_INCLUDE_DIRS})
    target_link_libraries (veloc-modules ${AXL_LIBRARIES})
endif()

# Install libraries
install (TARGETS veloc-modules
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
