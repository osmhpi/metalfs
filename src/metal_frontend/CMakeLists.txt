SET(METAL_FS_SHARED_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/known_operators.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/known_operators.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/buffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/socket.hpp
)

add_subdirectory(messages)
add_subdirectory(operator_agent)
add_subdirectory(metal_fs)
