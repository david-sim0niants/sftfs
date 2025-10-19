option(SFTFS_FUNCTION_TRACING "Tracing specific function entries and exits." OFF)

set(TARGET_NAME sftfs_common)
add_library(${TARGET_NAME} OBJECT logging.c)
target_include_directories(${TARGET_NAME} PUBLIC ${ROOT_INCLUDE_DIR})
target_compile_options(${TARGET_NAME} PUBLIC -Wall -Wextra -Werror=format)

set(TARGET_NAME sftfs_core)
add_library(${TARGET_NAME} OBJECT core.c)
target_include_directories(${TARGET_NAME} PRIVATE ${ROOT_INCLUDE_DIR})
target_link_libraries(${TARGET_NAME} PUBLIC -lfuse3)
target_link_libraries(${TARGET_NAME} PRIVATE sftfs_common sftfs_endp)

set(TARGET_NAME sftfs_ssh)
add_library(${TARGET_NAME} OBJECT ssh.c ssh_cli.c password_prompt.c)
target_include_directories(${TARGET_NAME} PRIVATE ${ROOT_INCLUDE_DIR})
target_link_libraries(${TARGET_NAME} PUBLIC -lssh)

add_subdirectory(abs)
add_subdirectory(endp)
