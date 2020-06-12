function(veloc_add_test name args outputs)

  ### Change value here -- will run 1 task per node
  set(parallel_nodes 4)

  # job launcher
  if(${VELOC_RESOURCE_MANAGER} STREQUAL "NONE")
    # Assume mpirun if no resource manager in place
    set(config_file veloc_slurm.cfg)
    set(test_param_s mpirun -np 1)
    set(test_param_p mpirun -np ${parallel_nodes})
  elseif(${VELOC_RESOURCE_MANAGER} STREQUAL "SLURM")
    set(config_file veloc_slurm.cfg)
    set(test_param_s srun -N 1 -n 1)
    set(test_param_p srun -N ${parallel_nodes} -n ${parallel_nodes})
  endif()

  # Serial Tests
  add_test(NAME serial_${name}_start COMMAND ${test_param_s} ./${name} ${args} ${config_file} 0)

  add_test(NAME serial_${name}_restart COMMAND ${test_param_s} ./${name} ${args} ${config_file} 1)
  set_property(TEST serial_${name}_restart APPEND PROPERTY DEPENDS serial_${name}_start)

  add_test(NAME serial_${name}_cleanup COMMAND ${CMAKE_CURRENT_DESTINATION_DIR}/test_cleanup.sh )
  set_property(TEST serial_${name}_cleanup APPEND PROPERTY DEPENDS serial_${name}_restart)

  # Parallel Tests
  add_test(NAME parallel_${name}_inital COMMAND ${test_param_p} ./${name} ${args} ${config_file} 0)
  add_test(NAME parallel_${name}_simulate_node_file_loss COMMAND ${test_param_p} ./test_simulate_node_file_loss.sh )
  set_property(TEST parallel_${name}_simulate_node_file_loss APPEND PROPERTY DEPENDS parallel_${name}_start)
  add_test(NAME parallel_${name}_restart COMMAND ${test_param_p} ./${name} ${args} ${config_file} 1)
  set_property(TEST parallel_${name}_restart APPEND PROPERTY DEPENDS parallel_${name}_simulate_node_file_loss)

  add_test(NAME parallel_${name}_cleanup COMMAND ${test_param_p} ./test_cleanup.sh )
  set_property(TEST parallel_${name}_cleanup APPEND PROPERTY DEPENDS parallel_${name}_restart)

endfunction(veloc_add_test)
