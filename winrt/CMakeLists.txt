cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

if(NOT WIN32)
    message(FATAL_ERROR "This projects relies on Windows Runtime components. Windows only.")
endif()

project(WinRTExamples
    VERSION 1.0
    DESCRIPTION "VC++ C++20 WinRT coroutine examples"
    HOMEPAGE_URL https://github.com/MiguelBarro/CppCoroutines 
    LANGUAGES CXX)

# User set up
option(USE_SDK_HEADERS "Use the Windows SDK provided headers instead of the repo ones" OFF)

# Create common setup for the executables
add_library(winrt_common_setup INTERFACE)
target_compile_features(winrt_common_setup INTERFACE cxx_std_20)

if(NOT USE_SDK_HEADERS)
    target_include_directories(winrt_common_setup INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/projection
    )
endif()

# compiler specific options
if(MSVC)
    target_compile_options(winrt_common_setup INTERFACE /W4 /WX /wd4834)

    # Windows 8 (minimum windows supporting WinRT)
    target_compile_definitions(winrt_common_setup INTERFACE _WIN32_WINNT=0x0602)

    # Rely on real windows components
    target_link_libraries(winrt_common_setup INTERFACE windowsapp.lib)
endif()

# Clang may run on any platform
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(winrt_common_setup INTERFACE
        -Wno-unknown-warning-option
        -Wno-deprecated-declarations
        -Wno-sign-compare
        -Wno-unused-lambda-capture
        -Wno-cast-function-type-mismatch
    )
endif()

# customize binary
set_target_properties(winrt_common_setup PROPERTIES RELEASE_POSTFIX "-${PROJECT_VERSION}")
set_target_properties(winrt_common_setup PROPERTIES DEBUG_POSTFIX "-${PROJECT_VERSION}d")

# simple asio modified performance tests
# the number of background threads is managed by the process default threadpool
# because they rely on the default WinRT awaiters
add_executable(winrt_simple_client winrt_simple_client.cpp)
target_link_libraries(winrt_simple_client PRIVATE winrt_common_setup)
add_executable(winrt_simple_server winrt_simple_server.cpp)
target_link_libraries(winrt_simple_server PRIVATE winrt_common_setup)

# install
install(
    TARGETS
        winrt_simple_client
        winrt_simple_server
    RUNTIME DESTINATION .
)
