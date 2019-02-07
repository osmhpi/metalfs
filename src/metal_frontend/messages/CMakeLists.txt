file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ProtoFiles})

add_library(messages STATIC ${PROTO_HDRS} ${PROTO_SRCS})

target_include_directories(messages
    PUBLIC
    ${PROTOBUF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(messages PUBLIC ${PROTOBUF_LIBRARIES})