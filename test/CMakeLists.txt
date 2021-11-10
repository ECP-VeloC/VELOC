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

add_test("parallel_veloc_chkpt_file_test_multi_ver_inital" "srun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-N4" "-n4" "../veloc_2chkpt_file_test" "256" "veloc.cfg" "0")

#add_test("parallel_veloc_chkpt_file_test_simulate_second_chkpoint_loss" "srun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-N1" "-n1" "./test_simulate_second_chkpoint_loss.sh")
#set_property(TEST parallel_veloc_chkpt_file_test_simulate_second_chkpoint_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_ver_inital)

#add_test("parallel_veloc_chkpt_file_test_multi_ver_restart" "srun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-N4" "-n4" "../veloc_2chkpt_file_test" "256" "veloc.cfg" "1")
#set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_simulate_second_chkpoint_loss)
#set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})

add_test("parallel_veloc_chkpt_file_test_multi_ver_cleanup" "srun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-N4" "-n4" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_ver_restart)

