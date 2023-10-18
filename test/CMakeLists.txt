if( ($ENV{HOSTNAME} MATCHES "uan") OR ($ENV{NAME} MATCHES "uan") )
        set(VELOC_PERSISTENT_PATH "/lus/gila/projects/CSC250STDM14_CNDA/gregK/persistent")
        set(VELOC_SCRATCH_PATH "/dev/shm/greg_scratch")
	set(MPI_EXEC "mpirun")
	set(MPI_PARAM1 "--ppn")
	set(MPI_PARAM2 "--np")
endif()	
set(THREADED "true")
set(MODE "async")
file(WRITE ${CMAKE_BINARY_DIR}/test/veloc.cfg "threaded = ${THREADED}\n")
file(APPEND ${CMAKE_BINARY_DIR}/test/veloc.cfg "persistent = ${VELOC_PERSISTENT_PATH}\n")
file(APPEND ${CMAKE_BINARY_DIR}/test/veloc.cfg "scratch = ${VELOC_SCRATCH_PATH}\n")
file(APPEND ${CMAKE_BINARY_DIR}/test/veloc.cfg "mode = ${MODE}\n")

FILE(COPY ${CMAKE_SOURCE_DIR}/test/test_cleanup.sh DESTINATION ${CMAKE_BINARY_DIR}/test FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_SOURCE_DIR}/test/test_simulate_node_file_loss.sh DESTINATION ${CMAKE_BINARY_DIR}/test FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_SOURCE_DIR}/test/test_simulate_scratch_and_persistent_loss.sh DESTINATION ${CMAKE_BINARY_DIR}/test FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_SOURCE_DIR}/test/test_simulate_multilevel_corruption.sh DESTINATION ${CMAKE_BINARY_DIR}/test FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_SOURCE_DIR}/test/test_simulate_second_chkpoint_loss.sh DESTINATION ${CMAKE_BINARY_DIR}/test FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)


ADD_EXECUTABLE(veloc_performance_mem_test veloc_performance_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_performance_mem_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_chkpt_mem_test veloc_chkpt_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_mem_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_chkpt_file_test veloc_chkpt_file_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_file_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_chkpt_mem_multilevel_corruption_test veloc_chkpt_mem_multilevel_corruption_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_mem_multilevel_corruption_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_chkpt_file_multilevel_corruption_test veloc_chkpt_file_multilevel_corruption_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_file_multilevel_corruption_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_2chkpt_mem_test veloc_2chkpt_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_2chkpt_mem_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_2chkpt_file_test veloc_2chkpt_file_test.c)
TARGET_LINK_LIBRARIES(veloc_2chkpt_file_test ${MPI_C_LIBRARIES} veloc-client)

message("-- HOSTNAME environment variable is set to: " $ENV{HOSTNAME})
message(INFO "${MPIEXEC_EXECUTABLE}")
#set(num_nodes 4)
set(num_nodes 2)
set(ranks_per_nodes 1 2 4 8 16 32)

add_test(
    NAME
       "serial_veloc_chkpt_restart_mem_test"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 1"
	)
	set_property(TEST serial_veloc_chkpt_restart_mem_test APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_restart_mem_test"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_restart_mem_test APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "serial_veloc_chkpt_restart_file_test"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 1"
	)
	set_property(TEST serial_veloc_chkpt_restart_file_test APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_restart_file_test"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_restart_file_test APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_mem_test_simulate_node_loss"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_simulate_node_file_loss.sh ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_mem_test_simulate_node_loss APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_file_test_simulate_node_loss"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_simulate_node_file_loss.sh ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_file_test_simulate_node_loss APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_mem_test_xor_simulate_scratch_and_persistent_loss"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/test_simulate_scratch_and_persistent_loss.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_mem_test_xor_simulate_scratch_and_persistent_loss APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_file_test_xor_simulate_scratch_and_persistent_loss"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} 1 ${CMAKE_BINARY_DIR}/test/test_simulate_scratch_and_persistent_loss.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_file_test_xor_simulate_scratch_and_persistent_loss APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_mem_test_multi_lev_corr"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_multilevel_corruption_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_mem_multilevel_corruption_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_file_test_multi_lev_corr"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_multilevel_corruption_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_chkpt_file_multilevel_corruption_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

add_test(
    NAME
       "parallel_veloc_chkpt_mem_test_multi_chkpt_version"
    COMMAND
    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_2chkpt_mem_test 256 veloc.cfg 0; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_simulate_second_chkpoint_loss.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_2chkpt_mem_test 256 veloc.cfg 1"
	)
	set_property(TEST parallel_veloc_chkpt_mem_test_multi_chkpt_version APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")

#parallel_veloc_chkpt_file_test_multi_chkpt_version test wont run on many clusters, because file checkpoints are written to HOME file system, and the persistent directory on LUSTRE only has the sym links
#add_test(
#    NAME
#       "parallel_veloc_chkpt_file_test_multi_chkpt_version"
#    COMMAND
#    bash -c "${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_cleanup.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
#       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_2chkpt_file_test 256 veloc.cfg 0; \
#       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/test_simulate_second_chkpoint_loss.sh ${VELOC_PERSISTENT_PATH} ${VELOC_SCRATCH_PATH}; \
#       ${MPI_EXEC} ${MPI_PARAM2} ${num_nodes} ${CMAKE_BINARY_DIR}/test/veloc_2chkpt_file_test 256 veloc.cfg 1"
#	)
#	set_property(TEST parallel_veloc_chkpt_file_test_multi_chkpt_version APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")
