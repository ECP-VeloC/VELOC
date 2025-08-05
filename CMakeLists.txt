cmake_minimum_required (VERSION 3.14)
project(VELOC VERSION 1.8)

##### Global settings
include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
add_definitions(-O2 -g -Wall -D__BENCHMARK -D__ASSERT -D__INFO)
set(CMAKE_CXX_STANDARD 17)

##### Configuration setting
set(COMM_QUEUE "ipc_queue" CACHE STRING "Communication protocol between client library and active backend")
set_property(CACHE COMM_QUEUE PROPERTY STRINGS ipc_queue socket_queue thallium_queue)
set(POSIX_IO "posix_io" CACHE STRING "POSIX transfer method between scratch and persistent")
set_property(CACHE POSIX_IO PROPERTY STRINGS direct rw)

##### Set preprocessor definitions
if (${POSIX_IO} STREQUAL "direct")
    add_definitions(-DWITH_POSIX_DIRECT)
else()
    add_definitions(-DWITH_POSIX_RW)
endif()

##### Find dependencies

# PThreads
find_package(Threads REQUIRED)
set(COMM_QUEUE_LIBRARIES Threads::Threads)

# Communication protocol
if (${COMM_QUEUE} STREQUAL "ipc_queue")
    # Boost is required for ipc_queue
    find_package(Boost CONFIG REQUIRED)
    list(APPEND COMM_QUEUE_LIBRARIES Boost::boost)
elseif (${COMM_QUEUE} STREQUAL "thallium_queue")
    # Thallium is required for thallium_queue
    find_package(thallium REQUIRED)
    list(APPEND COMM_QUEUE_LIBRARIES "thallium")
endif ()
configure_file(${PROJECT_SOURCE_DIR}/src/common/comm_queue.in
               ${PROJECT_SOURCE_DIR}/src/common/comm_queue.hpp
               @ONLY)

# OpenSSL
find_package(OpenSSL REQUIRED)

# MPI
find_package(MPI REQUIRED)

# ER
find_package(er REQUIRED)

# AXL
find_package(axl)
if(axl_FOUND)
  add_definitions(-DWITH_AXL)
endif()

# DAOS
find_package(DAOS)
if(DAOS_FOUND)
  include_directories(${DAOS_INCLUDE_DIRS})
  add_definitions(-DWITH_DAOS)
endif()

# -----------------------------------------------------------------------------------
# Main source code
add_subdirectory(src)

# -----------------------------------------------------------------------------------
# Testing infrastructure

find_package(cereal)
find_package(Bitsery CONFIG)
find_package(Boost CONFIG COMPONENTS serialization)
if (cereal_FOUND AND Bitsery_FOUND AND Boost_FOUND)
  set(SERIALIZATION_LIBRARIES Boost::serialization)
  message(STATUS "Building with the following serialization libraries: cereal bitsery boost")
else()
  message(STATUS "Some or all serialization libraries (cereal, bitsery, boost) required for C++ tests not found, skipping")
endif()

enable_testing()
add_subdirectory(test)

# -----------------------------------------------------------------------------------
# Install export sets and configs
install(EXPORT velocTargets
        FILE velocTargets.cmake
        NAMESPACE veloc::
        DESTINATION share/veloc/cmake)

# Add the export set to the build directory
export(EXPORT velocTargets NAMESPACE veloc:: FILE velocTargets.cmake)

# Setup the configs
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/velocConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/velocConfig.cmake
                              INSTALL_DESTINATION share/veloc/cmake)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/velocConfigVersion.cmake
                                 COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/velocConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/velocConfigVersion.cmake
        DESTINATION share/veloc/cmake)
