SET(METAL_FS_SHARED_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/known_afus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/known_afus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/buffer.h
)

add_subdirectory(afu_agent)
add_subdirectory(metal_fs)
