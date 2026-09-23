include_guard(DIRECTORY)

# Find the asio library
if(NOT DEFINED ENV{TMP})
    set(ENV{TMP} "/tmp")
endif()

set(CPPWINRT_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "Path to the cppwinrt source directory")
set(CPPWINRT_PATH_SUFFIXES Microsoft.Windows.CppWinRT.3.0.260818.1/bin CACHE STRING "Path suffixes to locate cppwinrt")

find_path(CPPWINRT_BIN_DIR "cppwinrt.exe" PATHS ${CPPWINRT_SRC_DIR} PATH_SUFFIXES ${CPPWINRT_PATH_SUFFIXES})

if(NOT CPPWINRT_BIN_DIR)
    # check nuget is available (on linux via apt)
    find_program(NUGET_EXECUTABLE nuget)
    find_program(DOTNET_EXECUTABLE dotnet)

    if(NUGET_EXECUTABLE)
        # Install asio
        message(WARNING "Could not find cppwinrt.exe proceeding to install from nuget package")
        execute_process(COMMAND ${NUGET_EXECUTABLE} install Microsoft.Windows.CppWinRT
            -Version 3.0.260818.1 -OutputDirectory "${CPPWINRT_SRC_DIR}"
        )
    elseif(DOTNET_EXECUTABLE)
        # Install asio
        message(WARNING "Could not find asio.hpp proceeding to install from nuget package")
        set(DUMMY_PROJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dummy)

        execute_process(COMMAND ${DOTNET_EXECUTABLE} new classlib -n dummy -o ${DUMMY_PROJECT_DIR})
        execute_process( COMMAND ${DOTNET_EXECUTABLE} package add Microsoft.Windows.CppWinRT --version 3.0.260818.1
            --package-directory "${CPPWINRT_SRC_DIR}" --project "${DUMMY_PROJECT_DIR}"
        )
        execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf ${DUMMY_PROJECT_DIR})
    else()
        message(FATAL_ERROR "Could neither find cppwinrt.exe nor install via nuget/dotnet")
    endif()

    # Check installation worked out
    find_path(CPPWINRT_BIN_DIR "cppwinrt.exe" PATHS ${CPPWINRT_SRC_DIR} PATH_SUFFIXES ${CPPWINRT_PATH_SUFFIXES})
    find_program(CPPWINRT_EXECUTABLE cppwinrt ${CPPWINRT_BIN_DIR})
    if(NOT CPPWINRT_EXECUTABLE)
        message(FATAL_ERROR "cppwinrt nuget installation failed")
    endif()
endif()
