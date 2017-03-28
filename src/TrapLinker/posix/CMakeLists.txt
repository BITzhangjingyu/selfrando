
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/TrapInfo
    ${CMAKE_SOURCE_DIR}/src/TrapLinker
    ${CMAKE_SOURCE_DIR}/src/Support
    ${CMAKE_SOURCE_DIR}/src/Support/posix
    )

add_executable(traplinker
    ELF.cpp
    TrapLinker.cpp
    arch/${SR_ARCH}/ELF.cpp
    Object.h
    LinkerOptions.inc
    )

# TODO: allow custom libelf path
target_link_libraries(traplinker elf randolib_support)
target_compile_options(traplinker PRIVATE -static)

add_custom_command(
    OUTPUT LinkerOptions.inc 
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gen_options.py
        LinkerOptions.inc
        "${CMAKE_CURRENT_SOURCE_DIR}/LinkerOptions.table"
        "${CMAKE_CURRENT_SOURCE_DIR}/IgnoredOptions.table"
    DEPENDS LinkerOptions.table IgnoredOptions.table
    VERBATIM
    COMMENT "Generate linker options table"
    )

add_custom_command(TARGET traplinker
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink traplinker ld
    COMMAND ${CMAKE_COMMAND} -E create_symlink traplinker ld.bfd
    COMMAND ${CMAKE_COMMAND} -E create_symlink traplinker ld.gold
    BYPRODUCTS ld ld.bfd ld.gold
    VERBATIM
    COMMENT "Creating ld symlinks"
    )

add_dependencies(traplinker randolib_support)
# TODO: install the files + symlinks