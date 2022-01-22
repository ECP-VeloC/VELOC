# - Try to find DAOS
# Once done this will define
#  DAOS_FOUND - System has libDAOS
#  DAOS_INCLUDE_DIRS - The libDAOS include directories
#  DAOS_LIBRARIES - The libraries needed to use libDAOS

FIND_PATH(WITH_DAOS_PREFIX
    NAMES include/daos.h
)

FIND_LIBRARY(DAOS_LIBRARIES
    NAMES daos
    HINTS ${WITH_DAOS_PREFIX}/lib
)

FIND_PATH(DAOS_INCLUDE_DIRS
    NAMES daos.h
    HINTS ${WITH_DAOS_PREFIX}/include
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DAOS DEFAULT_MSG
    DAOS_LIBRARIES
    DAOS_INCLUDE_DIRS
)

# Hide these vars from ccmake GUI
MARK_AS_ADVANCED(
	DAOS_LIBRARIES
	DAOS_INCLUDE_DIRS
)
