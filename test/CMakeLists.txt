FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/veloc_slurm.cfg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cleanup.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_node_file_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

ADD_EXECUTABLE(veloc_chkpt_mem_test veloc_chkpt_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_mem_test ${MPI_C_LIBRARIES} veloc-client)
VELOC_ADD_TEST(veloc_chkpt_mem_test 256 "")

ADD_EXECUTABLE(veloc_chkpt_file_test veloc_chkpt_file_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_file_test ${MPI_C_LIBRARIES} veloc-client)
VELOC_ADD_TEST(veloc_chkpt_file_test 256 "")

ADD_EXECUTABLE(veloc_chkpt_mem_test_kill veloc_chkpt_mem_kill_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_mem_test_kill ${MPI_C_LIBRARIES} veloc-client)
VELOC_ADD_TEST(veloc_chkpt_mem_test_kill 256 "")

ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
