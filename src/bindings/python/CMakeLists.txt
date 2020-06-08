# Install python
install (CODE "execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/bindings/python COMMAND python3 setup.py install --user)")
