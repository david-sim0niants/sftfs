set(TARGET_NAME sftfs_cache)
add_library(${TARGET_NAME} STATIC cache.c list.c file.c attr.c)
target_link_libraries(${TARGET_NAME} PRIVATE sftfs_common sftfs_abs)
