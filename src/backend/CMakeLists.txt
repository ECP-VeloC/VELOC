# Comment as needed to reduce build time
add_executable (backend main.cpp)

# Link the executable to the necessary libraries.
target_link_libraries (backend ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

# Install executables
install (TARGETS backend
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
