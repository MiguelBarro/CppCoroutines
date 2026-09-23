include_guard(DIRECTORY)

# Find the asio library
if(NOT DEFINED ENV{TMP})
    set(ENV{TMP} "/tmp")
endif()

set(ASIO_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "Path to the asio source directory")
set(ASIO_PATH_SUFFIXES asio.1.10.8/build/native/include;asio/1.10.8/build/native/include CACHE STRING "Path suffixes to locate asio.hpp")

find_path(ASIO_INCLUDE_DIR "asio.hpp" PATHS ${ASIO_SRC_DIR} PATH_SUFFIXES ${ASIO_PATH_SUFFIXES})

if(NOT ASIO_INCLUDE_DIR)
    # check nuget is available (on linux via apt)
    find_program(NUGET_EXECUTABLE nuget)
    find_program(DOTNET_EXECUTABLE dotnet)

    if(NUGET_EXECUTABLE)
        # Install asio
        message(WARNING "Could not find asio.hpp proceeding to install from nuget package")
        execute_process(COMMAND ${NUGET_EXECUTABLE} install asio
            -Version 1.10.8 -OutputDirectory "${ASIO_SRC_DIR}"
        )
    elseif(DOTNET_EXECUTABLE)
        # Install asio
        message(WARNING "Could not find asio.hpp proceeding to install from nuget package")
        set(DUMMY_PROJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dummy)

        execute_process(COMMAND ${DOTNET_EXECUTABLE} new classlib -n dummy -o ${DUMMY_PROJECT_DIR})
        execute_process( COMMAND ${DOTNET_EXECUTABLE} package add asio --version 1.10.8
            --package-directory "${ASIO_SRC_DIR}" --project "${DUMMY_PROJECT_DIR}"
        )
        execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf ${DUMMY_PROJECT_DIR})
    else()
        message(FATAL_ERROR "Could neither find asio.hpp nor install via nuget/dotnet")
    endif()

    # Check installation worked out
    find_path(ASIO_INCLUDE_DIR "asio.hpp" PATHS ${ASIO_SRC_DIR} PATH_SUFFIXES ${ASIO_PATH_SUFFIXES})
    if(NOT ASIO_INCLUDE_DIR)
        message(FATAL_ERROR "asio nuget installation failed")
    endif()
endif()

