set(
    SOURCES
    metal.c
)

set(
    LIBRARIES
    sqlite3
)

add_library(libmetal STATIC ${SOURCES})
target_link_libraries(libmetal ${LIBRARIES})
