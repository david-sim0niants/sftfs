set(TARGET_NAME ut)
add_executable(${TARGET_NAME} htable.c cache/list.c)
target_link_libraries(${TARGET_NAME} PRIVATE ut_main sftfs_abs sftfs_common sftfs_cache)
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME run)
