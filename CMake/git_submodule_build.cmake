macro(git_submodule_build _project_name)
    set(options INSTALL_SUB_DEPS)
    set(oneValueArgs "")
    set(multiValueArgs CMAKE_ARGS)
    cmake_parse_arguments(${_project_name} "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    message("-- Build submodule '${_project_name}' at ${CMAKE_SOURCE_DIR}/remote/${_project_name}")

    set(${_project_name}_command
            -G ${CMAKE_GENERATOR}
            .
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/fakeroot
            -B${CMAKE_BINARY_DIR}/remote/${_project_name}
            )

    list(APPEND ${_project_name}_command ${${_project_name}_CMAKE_ARGS})

    execute_process(COMMAND ${CMAKE_COMMAND} ${${_project_name}_command}
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/remote/${_project_name}
            )

    if(result)
        message(FATAL_ERROR "CMake step for ${_project_name} failed: ${result}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} --build . --target install
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/remote/${_project_name}
            )

    if(result)
        message(FATAL_ERROR "Build step for ${_project_name} failed: ${result}")
    endif()

    if (${_project_name}_INSTALL_SUB_DEPS)
        message("Install sub-dependenies of ${_project_name}...")

        file(COPY "${CMAKE_BINARY_DIR}/remote/${_project_name}/fakeroot/" DESTINATION "${CMAKE_BINARY_DIR}/fakeroot/")
    endif()

endmacro()