include_directories(
    ${FUSE_INCLUDE_DIR}
    ${SNAP_INCLUDE_DIR}
    ${CXL_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/include"
)

set(
    SOURCES

    storage.c
    storage.h
)

set(
    METAL_STORAGE_LIBRARIES
)

if(WITH_SNAP)
    SET(METAL_STORAGE_LIBRARIES
        ${METAL_STORAGE_LIBRARIES}
        ${SNAP_LIBRARY}
        ${CXL_LIBRARY}
    )
endif()

include_directories(
    ${SNAP_INCLUDE_DIR}
    ${CXL_INCLUDE_DIR}
)

add_library(metal_storage STATIC ${SOURCES})
target_link_libraries(metal_storage metal_operators ${METAL_STORAGE_LIBRARIES})
