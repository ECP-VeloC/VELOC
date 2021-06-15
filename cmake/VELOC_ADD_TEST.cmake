function(veloc_add_test name args outputs)

  ### Change value here -- will run 1 task per node
  set(parallel_nodes 4)

  # job launcher
  if(${VELOC_RESOURCE_MANAGER} STREQUAL "NONE")
    # Assume mpirun if no resource manager in place
    set(config_file veloc_slurm.cfg)
    set(test_param_s mpirun -np 1)
    set(test_param_p mpirun -np ${parallel_nodes})
  elseif(${VELOC_RESOURCE_MANAGER} STREQUAL "LSF")
    set(config_file veloc_lsf.cfg)
    set(test_param_s jsrun -n 1)
    set(test_param_p jsrun -r 1)
  elseif(${VELOC_RESOURCE_MANAGER} STREQUAL "SLURM")
    set(config_file veloc_slurm.cfg)
    set(test_param_s srun -N 1 -n 1)
    set(test_param_p srun -N ${parallel_nodes} -n ${parallel_nodes})
  endif()

#add_test(NAME start_backend_${name} COMMAND jsrun -r 1 ${CMAKE_INSTALL_FULL_BINDIR}/veloc-backend veloc_lsf.cfg )
#add_test(NAME start_backend_${name} COMMAND jsrun -r 1 ${CMAKE_INSTALL_FULL_BINDIR}/veloc-backend veloc_lsf.cfg & )

  # Serial Tests
 # add_test(NAME ${name}_start_backend COMMAND ./start_backend_serial.sh)
  add_test(NAME serial_${name}_start COMMAND ${test_param_s} ./${name} ${args} ${config_file} 0)
  #set_property(TEST serial_${name}_start APPEND PROPERTY DEPENDS ${name}_start_backend})
  set_property(TEST serial_${name}_start APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
  set_property(TEST serial_${name}_start APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

  add_test(NAME serial_${name}_restart COMMAND ${test_param_s} ./${name} ${args} ${config_file} 1)
  set_property(TEST serial_${name}_restart APPEND PROPERTY DEPENDS serial_${name}_start)
  set_property(TEST serial_${name}_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
  set_property(TEST serial_${name}_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

  #add_test(NAME serial_${name}_cleanup COMMAND ${CMAKE_CURRENT_DESTINATION_DIR}/test_cleanup.sh )
  #perform cleanup on all allocated nodes
  add_test(NAME serial_${name}_cleanup COMMAND ${test_param_p} ./test_cleanup.sh )
  set_property(TEST serial_${name}_cleanup APPEND PROPERTY DEPENDS serial_${name}_restart)

  #add_test(NAME ${name}_stop_backend COMMAND jsrun -n 1 sh -c "killall veloc-backend; exit 0" )
  #set_property(TEST ${name}_stop_backend APPEND PROPERTY DEPENDS TEST serial_${name}_cleanup)

  # Parallel Tests
  #add_test(NAME ${name}_start_backend_parallel COMMAND ./start_backend_parallel.sh )
  #set_property(TEST ${name}_start_backend_parallel PROPERTY DEPENDS TEST ${name}_stop_backend)
  add_test(NAME parallel_${name}_inital COMMAND ${test_param_p} ./${name} ${args} ${config_file} 0)
  #set_property(TEST parallel_${name}_inital APPEND PROPERTY DEPENDS ${name}_start_backend_parallel})
  set_property(TEST parallel_${name}_inital APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
  set_property(TEST parallel_${name}_inital APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")
  add_test(NAME parallel_${name}_simulate_node_file_loss COMMAND ${test_param_p} ./test_simulate_node_file_loss.sh)
  set_property(TEST parallel_${name}_simulate_node_file_loss APPEND PROPERTY DEPENDS parallel_${name}_start)
  add_test(NAME parallel_${name}_restart COMMAND ${test_param_p} ./${name} ${args} ${config_file} 1)
  set_property(TEST parallel_${name}_restart APPEND PROPERTY DEPENDS parallel_${name}_simulate_node_file_loss)
  set_property(TEST parallel_${name}_restart APPEND PROPERTY ENVIRONMENT VELOC_BIN=${CMAKE_INSTALL_FULL_BINDIR})
  set_property(TEST parallel_${name}_restart APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH="${CMAKE_INSTALL_FULL_LIBDIR}:$ENV{LD_LIBRARY_PATH}")

  add_test(NAME parallel_${name}_cleanup COMMAND ${test_param_p} ./test_cleanup.sh )
  set_property(TEST parallel_${name}_cleanup APPEND PROPERTY DEPENDS parallel_${name}_restart)
  #add_test(NAME ${name}_stop_backend_parallel COMMAND jsrun -r 1 sh -c "killall veloc-backend; exit 0" )
  #set_property(TEST ${name}_stop_backend_parallel APPEND PROPERTY DEPENDS TEST parallel_${name}_cleanup)

endfunction(veloc_add_test)
