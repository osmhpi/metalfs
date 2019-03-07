set(
    OPERATOR_AGENT_LIBRARIES

    pthread
    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)


SET(OPERATOR_AGENT_SOURCES

    operator_agent.cpp
)

add_executable(operator_agent ${OPERATOR_AGENT_SOURCES})
target_link_libraries(operator_agent messages ${OPERATOR_AGENT_LIBRARIES})
