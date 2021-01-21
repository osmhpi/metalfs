include(cxxopts.cmake)
include(googletest.cmake)
include(json.cmake)
include(spdlog.cmake)

if(WITH_SNAP)
    add_subdirectory(pslse)
    add_subdirectory(snap)
endif()

if(WITH_OCACCEL)
    add_subdirectory(ocse)
    add_subdirectory(oc-accel)
endif()

if(WITH_AWS_SV)
    add_subdirectory(aws-fpga)
endif()
