
#
# Find doxygen
#

find_package(Doxygen)
if(NOT DOXYGEN_FOUND)
    message(STATUS "Disabled generation of doxygen documentation (missing doxygen).")
    return()
endif()


#
# Target name
#

set(target api-docs)
message(STATUS "Doc ${target}")


#
# Input file
#

set(doxyfile_in doxyfile.in)


#
# Create documentation
#

# Set project variables
set(doxyfile            "${CMAKE_CURRENT_BINARY_DIR}/doxyfile")
set(doxyfile_directory  "${CMAKE_CURRENT_BINARY_DIR}/html")
set(doxyfile_html       "${doxyfile_directory}/index.html")

# Get filename and path of doxyfile
get_filename_component(name ${doxyfile_in} NAME)
get_filename_component(path ${doxyfile_in} PATH)
if(NOT path)
    set(path ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Configure doxyfile (if it is a real doxyfile already, it should simply copy the file)
set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${doxyfile_in} ${doxyfile} @ONLY)

# Invoke doxygen
add_custom_command(
    OUTPUT              ${doxyfile_html}
    DEPENDS             ${doxyfile} ${META_PROJECT_NAME}::metal-driver ${META_PROJECT_NAME}::metal-driver-messages ${META_PROJECT_NAME}::metal-driver-placeholder ${META_PROJECT_NAME}::metal-filesystem ${META_PROJECT_NAME}::metal-filesystem-pipeline ${META_PROJECT_NAME}::metal-pipeline
    WORKING_DIRECTORY   ${path}
    COMMAND             ${CMAKE_COMMAND} -E copy_directory ${path} ${doxyfile_directory} # ToDO, configure doxygen to use source as is
    COMMAND             ${DOXYGEN} \"${doxyfile}\"
    COMMENT             "Creating doxygen documentation."
)

# Declare target
add_custom_target(${target} ALL DEPENDS ${doxyfile_html})

# Sphinx

find_package(Sphinx REQUIRED)
set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

add_custom_target(Sphinx ALL
                  COMMAND ${SPHINX_EXECUTABLE} -b html
                  # Tell Breathe where to find the Doxygen output
                  -Dbreathe_projects.metalfs=${DOXYGEN_OUTPUT_DIRECTORY}/xml
                  ${SPHINX_SOURCE} ${SPHINX_BUILD}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating documentation with Sphinx")

add_dependencies(Sphinx ${target})
add_dependencies(docs ${target} Sphinx)

#
# Deployment
#

install(
    DIRECTORY ${doxyfile_directory}
    DESTINATION ${INSTALL_DOC}
    COMPONENT docs
)
