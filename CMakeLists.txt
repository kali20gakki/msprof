cmake_minimum_required(VERSION 3.14.1)

project(msprof)

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ################################### msprof #####################################
# set(MSPROF_HOME )

# add_subdirectory(collector)
# add_subdirectory(test)

# set(INSTALL_LIBRARY_DIR lib)
# set(INSTALL_CONFIG_DIR  ${CMAKE_CURRENT_BINARY_DIR}/lib/cmake)

# add_custom_target(profiling
#     ${CMAKE_COMMAND} -E echo "begin to build profiling"
#     && ${CMAKE_COMMAND} -E make_directory ${INSTALL_CONFIG_DIR}/profiling
#     COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/* ${INSTALL_CONFIG_DIR}/profiling/
#     && ${CMAKE_COMMAND} -E chdir ${INSTALL_CONFIG_DIR}/profiling/build/build_hiprof ./build_for_hiprof.sh "debug"
# )

# install(DIRECTORY
#     ${INSTALL_CONFIG_DIR}/profiling # install_config_dir is a relative path, must Splicing it while install
#     DESTINATION
#     ${INSTALL_LIBRARY_DIR}
#     OPTIONAL
# )
