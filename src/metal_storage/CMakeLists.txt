set(
    SOURCES
    
    storage.c
    storage.h
)

set(
    LIBRARIES
)


include_directories(
)

add_library(metal_storage STATIC ${SOURCES})
target_link_libraries(metal_storage ${LIBRARIES})
