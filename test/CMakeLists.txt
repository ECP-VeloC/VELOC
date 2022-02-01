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


add_test(serial_veloc_chkpt_mem_test_start "mpirun" "-np" "1" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "0")
set_property(TEST serial_veloc_chkpt_mem_test_start APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_mem_test_start APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


 
add_test("serial_veloc_chkpt_mem_test_restart" "mpirun" "-np" "1" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "1")
set_property(TEST serial_veloc_chkpt_mem_test_restart APPEND PROPERTY DEPENDS serial_veloc_chkpt_mem_test_start)
set_property(TEST serial_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


#add_test("serial_veloc_chkpt_mem_test_cleanup" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "2" "./test_cleanup.sh")
add_test("serial_veloc_chkpt_mem_test_cleanup" "mpirun" "-np" "1" "./test_cleanup.sh")
set_property(TEST serial_veloc_chkpt_mem_test_cleanup APPEND PROPERTY DEPENDS serial_veloc_chkpt_mem_test_restart)


add_test("parallel_veloc_chkpt_mem_test_inital" "mpirun" "-np" "2" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "0")
set_property(TEST parallel_veloc_chkpt_mem_test_inital APPEND PROPERTY DEPENDS serial_veloc_chkpt_mem_test_cleanup)
set_property(TEST parallel_veloc_chkpt_mem_test_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_mem_test_simulate_node_file_loss" "mpirun" "-np" "2" "./test_simulate_node_file_loss.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_simulate_node_file_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_inital)


add_test("parallel_veloc_chkpt_mem_test_restart" "mpirun" "-np" "2" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "1")
set_property(TEST parallel_veloc_chkpt_mem_test_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_simulate_node_file_loss)
set_property(TEST parallel_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_mem_test_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_restart)


add_test("serial_veloc_chkpt_file_test_start" "mpirun" "-np" "1" "../veloc_chkpt_file_test" "256" "veloc.cfg" "0")
set_property(TEST serial_veloc_chkpt_file_test_start APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_cleanup)
set_property(TEST serial_veloc_chkpt_file_test_start APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_file_test_start APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("serial_veloc_chkpt_file_test_restart" "mpirun" "-np" "1" "../veloc_chkpt_file_test" "256" "veloc.cfg" "1")
set_property(TEST serial_veloc_chkpt_file_test_restart APPEND PROPERTY DEPENDS serial_veloc_chkpt_file_test_start)
set_property(TEST serial_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("serial_veloc_chkpt_file_test_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST serial_veloc_chkpt_file_test_cleanup APPEND PROPERTY DEPENDS serial_veloc_chkpt_file_test_restart)


add_test("parallel_veloc_chkpt_file_test_inital" "mpirun" "-np" "2" "../veloc_chkpt_file_test" "256" "veloc.cfg" "0")
set_property(TEST parallel_veloc_chkpt_file_test_inital APPEND PROPERTY DEPENDS serial_veloc_chkpt_file_test_cleanup)
set_property(TEST parallel_veloc_chkpt_file_test_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_file_test_simulate_node_file_loss" "mpirun" "-np" "2" "./test_simulate_node_file_loss.sh")
set_property(TEST parallel_veloc_chkpt_file_test_simulate_node_file_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_inital)


add_test("parallel_veloc_chkpt_file_test_restart" "mpirun" "-np" "2" "../veloc_chkpt_file_test" "256" "veloc.cfg" "1")
set_property(TEST parallel_veloc_chkpt_file_test_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_simulate_node_file_loss)
set_property(TEST parallel_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_file_test_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_file_test_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_restart)


add_test("parallel_veloc_chkpt_mem_test_xor_inital" "mpirun" "-np" "2" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "0")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_cleanup)
set_property(TEST parallel_veloc_chkpt_mem_test_xor_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_xor_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_mem_test_xor_simulate_scratch_and_persistent_loss" "mpirun" "-np" "1" "./test_simulate_scratch_and_persistent_loss.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_simulate_scratch_and_persistent_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_inital)


add_test("parallel_veloc_chkpt_mem_test_xor_restart" "mpirun" "-np" "2" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "1")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_simulate_scratch_and_persistent_loss)
set_property(TEST parallel_veloc_chkpt_mem_test_xor_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_xor_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_mem_test_xor_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_restart)


add_test("parallel_veloc_chkpt_file_test_xor_inital" "mpirun" "-np" "2" "../veloc_chkpt_file_test" "256" "veloc.cfg" "0")
set_property(TEST parallel_veloc_chkpt_file_test_xor_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_cleanup)
set_property(TEST parallel_veloc_chkpt_file_test_xor_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_xor_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_file_test_xor_simulate_scratch_and_persistent_loss" "mpirun" "-np" "1" "./test_simulate_scratch_and_persistent_loss.sh")
set_property(TEST parallel_veloc_chkpt_file_test_xor_simulate_scratch_and_persistent_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_inital)


add_test("parallel_veloc_chkpt_file_test_xor_restart" "mpirun" "-np" "2" "../veloc_chkpt_file_test" "256" "veloc.cfg" "1")
set_property(TEST parallel_veloc_chkpt_file_test_xor_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_simulate_scratch_and_persistent_loss)
set_property(TEST parallel_veloc_chkpt_file_test_xor_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_xor_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_file_test_xor_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_file_test_xor_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_restart)


#add_test("parallel_veloc_chkpt_mem_test_multi_lev_corr_inital" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "2" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "0")
#set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_cleanup)
#set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})

#add_test("parallel_veloc_chkpt_mem_test_simulate_multilevel_corruption" "./test_simulate_multilevel_corruption.sh")
#set_property(TEST parallel_veloc_chkpt_mem_test_simulate_multilevel_corruption APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_lev_corr_inital)

#add_test("parallel_veloc_chkpt_mem_test_multi_lev_corr_restart" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "2" "../veloc_chkpt_mem_test" "256" "veloc.cfg" "1")
#set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_simulate_multilevel_corruption)
#set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
#

add_test("parallel_veloc_chkpt_mem_test_multi_lev_corr_inital" "mpirun" "-np" "2" "../veloc_chkpt_mem_multilevel_corruption_test" "256" "veloc.cfg" "0")
set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_cleanup)
set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_mem_test_multi_lev_corr_restart" "mpirun" "-np" "2" "../veloc_chkpt_mem_multilevel_corruption_test" "256" "veloc.cfg" "1")
set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_lev_corr_inital)
set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_mem_test_multi_lev_corr_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_multi_lev_corr_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_lev_corr_restart)

#add_test("parallel_veloc_chkpt_file_test_multi_lev_corr_inital" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "2" "../veloc_chkpt_file_test" "256" "veloc.cfg" "0")
#set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_lev_corr_cleanup)
#set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})

#add_test("parallel_veloc_chkpt_file_test_simulate_multilevel_corruption" "./test_simulate_multilevel_corruption.sh")
#set_property(TEST parallel_veloc_chkpt_file_test_simulate_multilevel_corruption APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_lev_corr_inital)

#add_test("parallel_veloc_chkpt_file_test_multi_lev_corr_restart" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "2" "../veloc_chkpt_file_test" "256" "veloc.cfg" "1")
#set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_simulate_multilevel_corruption)
#set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})


add_test("parallel_veloc_chkpt_file_test_multi_lev_corr_inital" "mpirun" "-np" "2" "../veloc_chkpt_file_multilevel_corruption_test" "256" "veloc.cfg" "0")
set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_lev_corr_cleanup)
set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_file_test_multi_lev_corr_restart" "mpirun" "-np" "2" "../veloc_chkpt_file_multilevel_corruption_test" "256" "veloc.cfg" "1")
set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_lev_corr_inital)
set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_file_test_multi_lev_corr_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_file_test_multi_lev_corr_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_lev_corr_restart)

add_test("parallel_veloc_chkpt_mem_test_multi_ver_inital" "mpirun" "-np" "2" "../veloc_2chkpt_mem_test" "256" "veloc.cfg" "0")
set_property(TEST parallel_veloc_chkpt_mem_test_multi_ver_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_lev_corr_cleanup)
set_property(TEST parallel_veloc_chkpt_mem_test_multi_ver_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_multi_ver_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_mem_test_simulate_second_chkpoint_loss" "mpirun" "-np" "1" "./test_simulate_second_chkpoint_loss.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_simulate_second_chkpoint_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_ver_inital)

add_test("parallel_veloc_chkpt_mem_test_multi_ver_restart" "mpirun" "-np" "2" "../veloc_2chkpt_mem_test" "256" "veloc.cfg" "1")
set_property(TEST parallel_veloc_chkpt_mem_test_multi_ver_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_simulate_second_chkpoint_loss)
set_property(TEST parallel_veloc_chkpt_mem_test_multi_ver_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_multi_ver_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")


add_test("parallel_veloc_chkpt_mem_test_multi_ver_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_multi_ver_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_ver_restart)

#add_test("parallel_veloc_chkpt_file_test_multi_ver_inital" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "2" "../veloc_2chkpt_file_test" "256" "veloc.cfg" "0")
#set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_multi_ver_cleanup)
#set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})

#add_test("parallel_veloc_chkpt_file_test_simulate_second_chkpoint_loss" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "1" "./test_simulate_second_chkpoint_loss.sh")
#set_property(TEST parallel_veloc_chkpt_file_test_simulate_second_chkpoint_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_ver_inital)

#add_test("parallel_veloc_chkpt_file_test_multi_ver_restart" "mpirun" "--export=LD_LIBRARY_PATH=../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}" "-np" "2" "../veloc_2chkpt_file_test" "256" "veloc.cfg" "1")
#set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_simulate_second_chkpoint_loss)
#set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})

#add_test("parallel_veloc_chkpt_file_test_multi_ver_cleanup" "mpirun" "-np" "2" "./test_cleanup.sh")
#set_property(TEST parallel_veloc_chkpt_file_test_multi_ver_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_multi_ver_restart)

