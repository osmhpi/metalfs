set(
    SOURCES

    op_blowfish_encrypt/operator.c
    op_blowfish_encrypt/operator.h
    op_blowfish_decrypt/operator.c
    op_blowfish_decrypt/operator.h
    op_read_file/operator.c
    op_read_file/operator.h
    op_read_mem/operator.c
    op_read_mem/operator.h
    op_change_case/operator.c
    op_change_case/operator.h
    op_passthrough/operator.c
    op_passthrough/operator.h
    op_write_file/operator.c
    op_write_file/operator.h
    op_write_mem/operator.c
    op_write_mem/operator.h

    operators.h
)

set(
    METAL_OPERATORS_LIBRARIES
    ${SNAP_LIBRARY}
)

include_directories(
    ${SNAP_INCLUDE_DIR}
)

add_library(metal_operators STATIC ${SOURCES})
target_link_libraries(metal_operators ${METAL_OPERATORS_LIBRARIES})
