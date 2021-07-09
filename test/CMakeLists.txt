FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/veloc_slurm.cfg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/veloc_lsf.cfg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_cleanup.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_node_file_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_simulate_scratch_and_persistent_loss.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/start_backend_serial.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/start_backend_parallel.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

add_test(serial_veloc_chkpt_mem_test_start "jsrun" "-n" "1" "../veloc_chkpt_mem_test" "256" "veloc_lsf.cfg" "0")
set_property(TEST serial_veloc_chkpt_mem_test_start APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_mem_test_start APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

 
add_test("serial_veloc_chkpt_mem_test_restart" "jsrun" "-n" "1" "../veloc_chkpt_mem_test" "256" "veloc_lsf.cfg" "1")
set_property(TEST serial_veloc_chkpt_mem_test_restart APPEND PROPERTY DEPENDS serial_veloc_chkpt_mem_test_start)
set_property(TEST serial_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("serial_veloc_chkpt_mem_test_cleanup" "jsrun" "-r" "1" "./test_cleanup.sh")
set_property(TEST serial_veloc_chkpt_mem_test_cleanup APPEND PROPERTY DEPENDS serial_veloc_chkpt_mem_test_restart)

add_test("parallel_veloc_chkpt_mem_test_inital" "jsrun" "-r" "1" "../veloc_chkpt_mem_test" "256" "veloc_lsf.cfg" "0")
set_property(TEST parallel_veloc_chkpt_mem_test_inital APPEND PROPERTY DEPENDS serial_veloc_chkpt_mem_test_cleanup)
set_property(TEST parallel_veloc_chkpt_mem_test_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_mem_test_simulate_node_file_loss" "jsrun" "-r" "1" "./test_simulate_node_file_loss.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_simulate_node_file_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_inital)

add_test("parallel_veloc_chkpt_mem_test_restart" "jsrun" "-r" "1" "../veloc_chkpt_mem_test" "256" "veloc_lsf.cfg" "1")
set_property(TEST parallel_veloc_chkpt_mem_test_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_simulate_node_file_loss)
set_property(TEST parallel_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_mem_test_cleanup" "jsrun" "-r" "1" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_restart)

add_test("serial_veloc_chkpt_file_test_start" "jsrun" "-n" "1" "../veloc_chkpt_file_test" "256" "veloc_lsf.cfg" "0")
set_property(TEST serial_veloc_chkpt_file_test_start APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_cleanup)
set_property(TEST serial_veloc_chkpt_file_test_start APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_file_test_start APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("serial_veloc_chkpt_file_test_restart" "jsrun" "-n" "1" "../veloc_chkpt_file_test" "256" "veloc_lsf.cfg" "1")
set_property(TEST serial_veloc_chkpt_file_test_restart APPEND PROPERTY DEPENDS serial_veloc_chkpt_file_test_start)
set_property(TEST serial_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST serial_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("serial_veloc_chkpt_file_test_cleanup" "jsrun" "-r" "1" "./test_cleanup.sh")
set_property(TEST serial_veloc_chkpt_file_test_cleanup APPEND PROPERTY DEPENDS serial_veloc_chkpt_file_test_restart)

add_test("parallel_veloc_chkpt_file_test_inital" "jsrun" "-r" "1" "../veloc_chkpt_file_test" "256" "veloc_lsf.cfg" "0")
set_property(TEST parallel_veloc_chkpt_file_test_inital APPEND PROPERTY DEPENDS serial_veloc_chkpt_file_test_cleanup)
set_property(TEST parallel_veloc_chkpt_file_test_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_file_test_simulate_node_file_loss" "jsrun" "-r" "1" "./test_simulate_node_file_loss.sh")
set_property(TEST parallel_veloc_chkpt_file_test_simulate_node_file_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_inital)

add_test("parallel_veloc_chkpt_file_test_restart" "jsrun" "-r" "1" "../veloc_chkpt_file_test" "256" "veloc_lsf.cfg" "1")
set_property(TEST parallel_veloc_chkpt_file_test_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_simulate_node_file_loss)
set_property(TEST parallel_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_file_test_cleanup" "jsrun" "-r" "1" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_file_test_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_restart)

add_test("parallel_veloc_chkpt_mem_test_xor_inital" "jsrun" "-r" "1" "../veloc_chkpt_mem_test" "256" "veloc_lsf.cfg" "0")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_cleanup)
set_property(TEST parallel_veloc_chkpt_mem_test_xor_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_xor_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_mem_test_xor_simulate_scratch_and_persistent_loss" "jsrun" "-n" "1" "./test_simulate_scratch_and_persistent_loss.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_simulate_scratch_and_persistent_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_inital)

add_test("parallel_veloc_chkpt_mem_test_xor_restart" "jsrun" "-r" "1" "../veloc_chkpt_mem_test" "256" "veloc_lsf.cfg" "1")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_inital)
set_property(TEST parallel_veloc_chkpt_mem_test_xor_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_mem_test_xor_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_mem_test_xor_cleanup" "jsrun" "-r" "1" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_mem_test_xor_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_restart)

add_test("parallel_veloc_chkpt_file_test_xor_inital" "jsrun" "-r" "1" "../veloc_chkpt_file_test" "256" "veloc_lsf.cfg" "0")
set_property(TEST parallel_veloc_chkpt_file_test_xor_inital APPEND PROPERTY DEPENDS parallel_veloc_chkpt_mem_test_xor_cleanup)
set_property(TEST parallel_veloc_chkpt_file_test_xor_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_xor_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_file_test_xor_simulate_scratch_and_persistent_loss" "jsrun" "-n" "1" "./test_simulate_scratch_and_persistent_loss.sh")
set_property(TEST parallel_veloc_chkpt_file_test_xor_simulate_scratch_and_persistent_loss APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_inital)

add_test("parallel_veloc_chkpt_file_test_xor_restart" "jsrun" "-r" "1" "../veloc_chkpt_file_test" "256" "veloc_lsf.cfg" "1")
set_property(TEST parallel_veloc_chkpt_file_test_xor_restart APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_inital)
set_property(TEST parallel_veloc_chkpt_file_test_xor_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
set_property(TEST parallel_veloc_chkpt_file_test_xor_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="../src/lib:${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

add_test("parallel_veloc_chkpt_file_test_xor_cleanup" "jsrun" "-r" "1" "./test_cleanup.sh")
set_property(TEST parallel_veloc_chkpt_file_test_xor_cleanup APPEND PROPERTY DEPENDS parallel_veloc_chkpt_file_test_xor_restart)

