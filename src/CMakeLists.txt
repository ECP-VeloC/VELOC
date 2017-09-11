# Header files need not be enumerated
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

#############
# LIB VELOC #
#############

# installed header files
LIST(APPEND v_install_headers
        veloc.h
)
INSTALL(FILES ${v_install_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Source files
SET(v_srcs
    veloc.c
)

# object files
ADD_LIBRARY(veloc_o OBJECT ${v_srcs})

# static library
ADD_LIBRARY(veloc STATIC $<TARGET_OBJECTS:veloc_o>)
TARGET_LINK_LIBRARIES(veloc ${VELOC_EXTERNAL_LIBS})
SET_TARGET_PROPERTIES(veloc PROPERTIES OUTPUT_NAME veloc CLEAN_DIRECT_OUTPUT 1)
INSTALL(TARGETS veloc DESTINATION ${CMAKE_INSTALL_LIBDIR})

# shared library
ADD_LIBRARY(veloc-shared SHARED $<TARGET_OBJECTS:veloc_o>)
TARGET_LINK_LIBRARIES(veloc-shared ${VELOC_EXTERNAL_LIBS})
SET_TARGET_PROPERTIES(veloc-shared PROPERTIES OUTPUT_NAME veloc CLEAN_DIRECT_OUTPUT 1)
INSTALL(TARGETS veloc-shared DESTINATION ${CMAKE_INSTALL_LIBDIR})
