FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/veloc.cfg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cleanup.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_node_file_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_scratch_and_persistent_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/start_backend_serial.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/start_backend_parallel.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_multilevel_corruption.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_second_chkpoint_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)


ADD_EXECUTABLE(veloc_chkpt_mem_test veloc_chkpt_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_mem_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_chkpt_file_test veloc_chkpt_file_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_file_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_2chkpt_mem_test veloc_2chkpt_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_2chkpt_mem_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_2chkpt_file_test veloc_2chkpt_file_test.c)
TARGET_LINK_LIBRARIES(veloc_2chkpt_file_test ${MPI_C_LIBRARIES} veloc-client)

add_test("veloc_performance_mem_test" "mpirun" "--ppn" "1" "--np" "8" "../veloc_performance_mem_test" "256" "veloc.cfg" "128" "1" perf_results.csv)
set_property(TEST veloc_performance_mem_test APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST veloc_performance_mem_test APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("veloc_performance_mem_test_cleanup" "mpirun" "--ppn" "1" "--np" "8" "./test_cleanup.sh")
set_property(TEST veloc_performance_mem_test_cleanup APPEND PROPERTY DEPENDS veloc_performance_mem_test)

add_test("veloc_performance_mem_test2" "mpirun" "--ppn" "2" "--np" "16" "../veloc_performance_mem_test" "256" "veloc.cfg" "128" "2" perf_results.csv)
set_property(TEST veloc_performance_mem_test2 APPEND PROPERTY DEPENDS veloc_performance_mem_test_cleanup)
set_property(TEST veloc_performance_mem_test2 APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST veloc_performance_mem_test2 APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("veloc_performance_mem_test2_cleanup" "mpirun" "--ppn" "1" "--np" "8" "./test_cleanup.sh")
set_property(TEST veloc_performance_mem_test2_cleanup APPEND PROPERTY DEPENDS veloc_performance_mem_test2)

add_test("veloc_performance_mem_test4" "mpirun" "--ppn" "4" "--np" "32" "../veloc_performance_mem_test" "256" "veloc.cfg" "128" "4" perf_results.csv)
set_property(TEST veloc_performance_mem_test4 APPEND PROPERTY DEPENDS veloc_performance_mem_test2_cleanup)
set_property(TEST veloc_performance_mem_test4 APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST veloc_performance_mem_test4 APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("veloc_performance_mem_test4_cleanup" "mpirun" "--ppn" "1" "--np" "8" "./test_cleanup.sh")
set_property(TEST veloc_performance_mem_test4_cleanup APPEND PROPERTY DEPENDS veloc_performance_mem_test4)

add_test("veloc_performance_mem_test8" "mpirun" "--ppn" "8" "--np" "64" "../veloc_performance_mem_test" "256" "veloc.cfg" "128" "8" perf_results.csv)
set_property(TEST veloc_performance_mem_test8 APPEND PROPERTY DEPENDS veloc_performance_mem_test4_cleanup)
set_property(TEST veloc_performance_mem_test8 APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST veloc_performance_mem_test8 APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("veloc_performance_mem_test8_cleanup" "mpirun" "--ppn" "1" "--np" "8" "./test_cleanup.sh")
set_property(TEST veloc_performance_mem_test8_cleanup APPEND PROPERTY DEPENDS veloc_performance_mem_test8)

add_test("veloc_performance_mem_test16" "mpirun" "--ppn" "16" "--np" "128" "../veloc_performance_mem_test" "256" "veloc.cfg" "128" "16" perf_results.csv)
set_property(TEST veloc_performance_mem_test16 APPEND PROPERTY DEPENDS veloc_performance_mem_test8_cleanup)
set_property(TEST veloc_performance_mem_test16 APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST veloc_performance_mem_test16 APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("veloc_performance_mem_test16_cleanup" "mpirun" "--ppn" "1" "--np" "8" "./test_cleanup.sh")
set_property(TEST veloc_performance_mem_test16_cleanup APPEND PROPERTY DEPENDS veloc_performance_mem_test16)

add_test("veloc_performance_mem_test32" "mpirun" "--ppn" "32" "--np" "256" "../veloc_performance_mem_test" "256" "veloc.cfg" "128" "32" perf_results.csv)
set_property(TEST veloc_performance_mem_test32 APPEND PROPERTY DEPENDS veloc_performance_mem_test16_cleanup)
set_property(TEST veloc_performance_mem_test32 APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST veloc_performance_mem_test32 APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("veloc_performance_mem_test32_cleanup" "mpirun" "--ppn" "1" "--np" "8" "./test_cleanup.sh")
set_property(TEST veloc_performance_mem_test32_cleanup APPEND PROPERTY DEPENDS veloc_performance_mem_test32)

