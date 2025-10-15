set(TARGET_NAME sftfs_abs)
add_library(${TARGET_NAME} OBJECT str.c htable.c)
target_link_libraries(${TARGET_NAME} PRIVATE sftfs_common)
