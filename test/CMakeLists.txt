PROJECT(VELOC_Tests)

# BUILD EXAMPLES

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

## File Interface
ADD_EXECUTABLE(heatdis_file heatdis_file.c heatdis_common.c)
TARGET_LINK_LIBRARIES(heatdis_file veloc)

## Memory Interface
ADD_EXECUTABLE(heatdis_mem heatdis_mem.c heatdis_common.c)
TARGET_LINK_LIBRARIES(heatdis_mem veloc)

## Both: File & Memory Interface
ADD_EXECUTABLE(heatdis_memfile heatdis_memfile.c heatdis_common.c)
TARGET_LINK_LIBRARIES(heatdis_memfile veloc)

# TESTING

## Move test cleanup script to build directory
# FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cleanup.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
# FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# add target: "make check"
# prints output from the ctests
ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
