add_library (veloc-client SHARED 
  veloc.cpp
  client.cpp
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)

# Link the executable to the necessary libraries.
target_link_libraries (veloc-client ${Boost_LIBRARIES} veloc-modules)

# Install libraries
install (TARGETS veloc-client
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install (FILES
  ${VELOC_SOURCE_DIR}/include/veloc.h
  DESTINATION include
)
