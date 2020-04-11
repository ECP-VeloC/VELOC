# - Try to find ER
# Once done this will define
#  ER_FOUND - System has libER
#  ER_INCLUDE_DIRS - The libER include directories
#  ER_LIBRARIES - The libraries needed to use libER

FIND_PATH(WITH_ER_PREFIX
    NAMES include/er.h
)

FIND_LIBRARY(ER_LIB NAMES er HINTS ${WITH_ER_PREFIX}/lib)
FIND_LIBRARY(RANKSTR_LIB NAMES rankstr HINTS ${WITH_ER_PREFIX}/lib)

LIST(APPEND ER_LIBRARIES ${ER_LIB} ${RANKSTR_LIB})

FIND_PATH(ER_INCLUDE_DIRS
    NAMES er.h
    HINTS ${WITH_ER_PREFIX}/include
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ER DEFAULT_MSG
    ER_LIBRARIES
    ER_INCLUDE_DIRS
)

# Hide these vars from ccmake GUI
MARK_AS_ADVANCED(
    ER_LIBRARIES
    ER_INCLUDE_DIRS
)
