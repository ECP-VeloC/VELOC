SET(common_bins
	veloc_check_node
	veloc_glob_hosts
)

FOREACH(bin IN ITEMS ${common_bins})
	VELOC_INSTALL_BIN(${bin})
ENDFOREACH(bin IN ITEMS ${common_bins})

## Configure & install .pm files
INSTALL(FILES veloc_hostlist.pm DESTINATION ${CMAKE_INSTALL_DATADIR}/veloc)
