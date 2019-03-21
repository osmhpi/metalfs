file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ProtoFiles})

# Disable warnings-as-errors for generated files
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS -Wno-error)

SET(MESSAGES_SOURCES
    buffer.cpp
    buffer.hpp
    message_header.cpp
    message_header.hpp
    socket.hpp
)


add_library(messages STATIC ${PROTO_HDRS} ${PROTO_SRCS} ${MESSAGES_SOURCES})

target_include_directories(messages
    PUBLIC
    ${PROTOBUF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(messages PUBLIC ${PROTOBUF_LIBRARIES})
