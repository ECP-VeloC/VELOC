add_library (veloc SHARED 
  veloc.cpp
  ${VELOC_SOURCE_DIR}/src/common/config.cpp
)

# Link the executable to the necessary libraries.
target_link_libraries (veloc ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

# Install libraries
install (TARGETS veloc
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install (FILES
  ${VELOC_SOURCE_DIR}/include/veloc.h
  DESTINATION include
)
