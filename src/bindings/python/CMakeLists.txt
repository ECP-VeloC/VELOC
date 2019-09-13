# Install python
install (CODE  "execute_process(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/bindings/python
                                COMMAND python3 setup.py install --user)")
