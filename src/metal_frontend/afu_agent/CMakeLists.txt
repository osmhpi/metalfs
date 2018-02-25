set(
    AFU_AGENT_LIBRARIES
)


SET(AFU_AGENT_SOURCES
    ${METAL_FS_SHARED_SOURCES}

    afu_agent.c
)

add_executable(afu_agent ${AFU_AGENT_SOURCES})
target_link_libraries(afu_agent metal_afus ${AFU_AGENT_LIBRARIES})
