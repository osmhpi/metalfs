set(
    OPERATOR_AGENT_LIBRARIES

    pthread
    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)


SET(OPERATOR_AGENT_SOURCES
    ${METAL_FS_SHARED_SOURCES}

    operator_agent.c
)

add_executable(operator_agent ${OPERATOR_AGENT_SOURCES})
target_link_libraries(operator_agent metal_operators ${OPERATOR_AGENT_LIBRARIES})
