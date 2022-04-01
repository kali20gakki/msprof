
set(INSTALL_LIBRARY_DIR lib)

set(stackcoreSrcList 
    stackcore.c
)

set(stackcoreHeaders
    ${TOP_DIR}/abl/libc_sec/include
)

add_library(stackcore SHARED
    ${stackcoreSrcList}
)

target_include_directories(stackcore PRIVATE
    ${stackcoreHeaders}
)

target_link_libraries(stackcore PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:adump_headers>
    c_sec
)

target_compile_options(stackcore PRIVATE
    -Werror
    -std=c99
    -fstack-protector-strong
    -fPIC
)

target_link_options(stackcore PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
)

target_compile_definitions(stackcore PRIVATE
    _GNU_SOURCE
)

install(TARGETS stackcore OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)
