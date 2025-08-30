cmake_minimum_required(VERSION 3.22.0)
cmake_policy(VERSION 3.22.0)

project(wave_tracer_docs)

option(BUILD_DOC "Build documentation" ON)

find_package(Doxygen)

if (BUILD_DOC)
if (DOXYGEN_FOUND)
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" ${CMAKE_MODULE_PATH})
    find_package(Sphinx REQUIRED)
    
    set(ROOT ${PROJECT_SOURCE_DIR}/..)

    file(GLOB_RECURSE ALL_HEADERS ${ROOT}/include/*.hpp)

    set(DOXYGEN_INPUT_DIR ${ROOT}/include)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
    set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    message(${DOXYGEN_OUTPUT_DIR})

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

    #Replace variables inside @@ with the current values
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    add_custom_command(
            OUTPUT ${DOXYGEN_INDEX_FILE}
            DEPENDS ${ALL_HEADERS}
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
            MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
            COMMENT "Generating docs")
    add_custom_target(docs ALL DEPENDS ${DOXYGEN_INDEX_FILE})

    set(SPHINX_SOURCE ${CMAKE_CURRENT_LIST_DIR}/source)
    set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
    set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

    file(MAKE_DIRECTORY ${SPHINX_BUILD})

    add_custom_command(
            OUTPUT ${SPHINX_INDEX_FILE}
            COMMAND ${SPHINX_EXECUTABLE} -b html
                -Dbreathe_projects.wave_tracer=${DOXYGEN_OUTPUT_DIR}/xml ${SPHINX_SOURCE} ${SPHINX_BUILD}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS
                ${SPHINX_SOURCE}/index.rst
                ${DOXYGEN_INDEX_FILE}
            MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
            COMMENT "Generating documentation with Sphinx")
    add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
endif (BUILD_DOC)
