# Function to configure binaries (file.in -> file) and install into bin directory
# Called from the subdirectories added below
FUNCTION(VELOC_INSTALL_BIN file)
	CONFIGURE_FILE(${file}.in ${file} @ONLY)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/${file}.in PROPERTIES GENERATED FALSE)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/${file} PROPERTIES GENERATED TRUE)

	INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${file} DESTINATION ${CMAKE_INSTALL_BINDIR})
ENDFUNCTION(VELOC_INSTALL_BIN file)

# Common Binaries
ADD_SUBDIRECTORY(common)

# Pick the Resource Manager
SET(cliveloc_dir_name "")
SET(cliveloc_run_name "")

IF(${VELOC_RESOURCE_MANAGER} STREQUAL "SLURM")
	SET(cliveloc_dir_name "SLURM")
	SET(cliveloc_run_name "veloc_srun")
	LIST(APPEND resource_manager_bins
		veloc_srun
	)
ENDIF(${VELOC_RESOURCE_MANAGER} STREQUAL "SLURM")

IF(${VELOC_RESOURCE_MANAGER} STREQUAL "LSF")
	SET(cliveloc_dir_name "LSF")
	SET(cliveloc_run_name "veloc_jsrun")
	LIST(APPEND resource_manager_bins
		veloc_jsrun
	)
ENDIF(${VELOC_RESOURCE_MANAGER} STREQUAL "LSF")

IF(${VELOC_RESOURCE_MANAGER} STREQUAL "ALPS")
	SET(cliveloc_dir_name "ALPS")
	SET(cliveloc_run_name "veloc_aprun")
	LIST(APPEND resource_manager_bins
		veloc_aprun
	)
ENDIF(${VELOC_RESOURCE_MANAGER} STREQUAL "ALPS")

IF(${VELOC_RESOURCE_MANAGER} STREQUAL "NONE")
	MESSAGE("No resource manager selected. Some of the VELOC command line interface will not be built.")
ELSE(${VELOC_RESOURCE_MANAGER} STREQUAL "NONE")
	# Each of the following must be implemented for all resource managers
	LIST(APPEND resource_manager_bins
		veloc_env
		veloc_list_down_nodes
	)

	FOREACH(bin IN ITEMS ${resource_manager_bins})
		VELOC_INSTALL_BIN(${cliveloc_dir_name}/${bin})
	ENDFOREACH(bin IN ITEMS ${resource_manager_bins})

	#INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${cliveloc_dir_name}/${cliveloc_run_name} DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME ${cliveloc_run_name})
ENDIF(${VELOC_RESOURCE_MANAGER} STREQUAL "NONE")
