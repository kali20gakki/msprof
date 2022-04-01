cmake_minimum_required(VERSION 3.14.1)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

################################################################
add_library(mmpa_headers INTERFACE)

target_include_directories(mmpa_headers INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/mmpa
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/mmpa/sub_inc
    )

################################################################
add_library(mmpa SHARED
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:src/mmpa_linux.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:src/mmpa_win.cpp>
)

target_include_directories(mmpa PRIVATE
    ${TOP_DIR}/inc
)

target_link_libraries(mmpa PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    c_sec
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:dl>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:rt>
)

target_compile_options(mmpa PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-fvisibility=hidden>
)

target_compile_definitions(mmpa PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    FUNC_VISIBILITY
)

set_target_properties(mmpa
    PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,libmmpa,mmpa>
)

################################################################
add_library(static_mmpa  STATIC
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:src/mmpa_linux.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:src/mmpa_win.cpp>
)

target_include_directories(static_mmpa PRIVATE
    ${TOP_DIR}/inc
)

target_compile_definitions(static_mmpa PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
)

target_compile_options(static_mmpa PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-fvisibility=hidden>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_link_libraries(static_mmpa PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:c_sec_headers>
)

set_target_properties(static_mmpa
    PROPERTIES
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,libstatic_mmpa,mmpa>
)

################################################################

set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

if(${TARGET_SYSTEM_NAME} STREQUAL "Windows")
install(TARGETS mmpa static_mmpa OPTIONAL
    EXPORT mmpa-targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

install(FILES $<TARGET_PDB_FILE:mmpa> DESTINATION ${INSTALL_LIBRARY_DIR} OPTIONAL)
else()
install(TARGETS mmpa static_mmpa OPTIONAL
    EXPORT mmpa-targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif()
