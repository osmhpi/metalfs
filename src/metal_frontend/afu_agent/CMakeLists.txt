SET(AFU_AGENT_SOURCES
    ../common/buffer.c

    afu_agent.c
)

add_executable(afu_agent ${AFU_AGENT_SOURCES})
