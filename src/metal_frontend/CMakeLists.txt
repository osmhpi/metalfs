SET(METAL_FS_SHARED_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/known_operators.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/known_operators.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/buffer.h
)

add_subdirectory(operator_agent)
add_subdirectory(metal_fs)
