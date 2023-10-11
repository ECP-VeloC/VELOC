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

#FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/veloc.cfg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cleanup.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_node_file_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_scratch_and_persistent_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/start_backend_serial.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/start_backend_parallel.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_multilevel_corruption.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_second_chkpoint_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)


ADD_EXECUTABLE(veloc_chkpt_mem_test veloc_chkpt_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_chkpt_mem_test ${MPI_C_LIBRARIES} veloc-client)
ADD_EXECUTABLE(veloc_performance_mem_test veloc_performance_mem_test.c)
TARGET_LINK_LIBRARIES(veloc_performance_mem_test ${MPI_C_LIBRARIES} veloc-client)
#ADD_EXECUTABLE(veloc_chkpt_file_test veloc_chkpt_file_test.c)
#TARGET_LINK_LIBRARIES(veloc_chkpt_file_test ${MPI_C_LIBRARIES} veloc-client)
#ADD_EXECUTABLE(veloc_2chkpt_mem_test veloc_2chkpt_mem_test.c)
#TARGET_LINK_LIBRARIES(veloc_2chkpt_mem_test ${MPI_C_LIBRARIES} veloc-client)
#ADD_EXECUTABLE(veloc_2chkpt_file_test veloc_2chkpt_file_test.c)
#TARGET_LINK_LIBRARIES(veloc_2chkpt_file_test ${MPI_C_LIBRARIES} veloc-client)
#set(CMAKE_C_COMPILER mpicxx)
#set(CMAKE_CPP_COMPILER mpicxx)
#add_compile_options(-g -DNDEBUG)
#add_executable(veloc_performance_mem_test)
#target_sources(veloc_performance_mem_test PRIVATE "veloc_performance_mem_test.c")
#target_link_libraries(veloc_performance_mem_test PRIVATE ${MPI_C_LIBRARIES} ../deploy/lib64 veloc-client)
#target_include_directories(veloc_performance_mem_test PRIVATE ../deploy/include)
message("-- HOSTNAME environment variable is set to: " $ENV{HOSTNAME})
message(INFO "${MPIEXEC_EXECUTABLE}")
set(num_nodes 4)
set(ranks_per_nodes 1 2 4 8 16 32)

foreach (ranks_per_node ${ranks_per_nodes})
	math(EXPR tot_ranks "${ranks_per_node} * ${num_nodes}")
	add_test(
	    NAME
	       "veloc_performance_mem_test${ranks_per_node}"
	    COMMAND
	       bash -c "rm -rf ${VELOC_PERSISTENT_PATH}/*; \
	       rm -rf ${VELOC_SCRATCH_PATH}/*; \
	       ${MPI_EXEC} ${MPI_PARAM1} ${ranks_per_node} ${MPI_PARAM2} ${tot_ranks} ${CMAKE_CURRENT_BINARY_DIR}/veloc_performance_mem_test 256 veloc.cfg 128 ${ranks_per_node} perf_results.csv"
        )
	set_property(TEST veloc_performance_mem_test${ranks_per_node} APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${PROJECT_SOURCE_DIR}/../deploy/lib:${PROJECT_SOURCE_DIR}/../deploy/lib64:${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}")
endforeach ()

