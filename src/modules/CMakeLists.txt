if(VELOC_LINK_STATIC)
  add_library (veloc-modules STATIC 
    module_manager.cpp
    client_watchdog.cpp transfer_module.cpp
    client_aggregator.cpp ec_module.cpp
    ${VELOC_SOURCE_DIR}/src/common/config.cpp
  )
  set_target_properties(veloc-modules PROPERTIES LINK_SEARCH_START_STATIC 1)
  set_target_properties(veloc-modules PROPERTIES LINK_SEARCH_END_STATIC 1)
else()
  add_library (veloc-modules SHARED
    module_manager.cpp
    client_watchdog.cpp transfer_module.cpp
    client_aggregator.cpp ec_module.cpp
    ${VELOC_SOURCE_DIR}/src/common/config.cpp
  )
endif()

target_link_libraries(veloc-modules ${ER_LIBRARIES} ${AXL_LIBRARIES} ${KVTREE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Install libraries
install (TARGETS veloc-modules
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
