cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(GorExamples
    VERSION 1.0
    DESCRIPTION "VC++ C++20 version of Gor Nishanov's CppCon 2017 examples"
    HOMEPAGE_URL https://github.com/GorNishanov/await.git 
    LANGUAGES CXX)

# Find the Threads library
find_package(Threads REQUIRED)

include(asio.cmake)

# Create common setup for the executables
add_library(gor_common_setup INTERFACE)
target_compile_features(gor_common_setup INTERFACE cxx_std_20)
target_link_libraries(gor_common_setup INTERFACE common_stl_module)

# compiler specific options
if(MSVC)
    target_compile_options(gor_common_setup INTERFACE /W4 /WX /wd4834)

    target_compile_definitions(gor_common_setup INTERFACE
        _WIN32_WINNT=0x0A00
        _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
    )
elseif(LINUX)
    target_compile_options(gor_common_setup INTERFACE
        -Wall -Wextra -pedantic -Werror
        -Wno-format-overflow
        -Wno-implicit-fallthrough
    )
elseif(APPLE)
    target_compile_options(gor_common_setup INTERFACE
        -Wall -Wextra -pedantic -Werror
    )
endif()

# Clang may run on any platform
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(gor_common_setup INTERFACE
        -Wno-unknown-warning-option
        -Wno-deprecated-declarations
        -Wno-sign-compare
        -Wno-unused-lambda-capture
        -Wno-cast-function-type-mismatch
    )
endif()

target_include_directories(gor_common_setup INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${ASIO_INCLUDE_DIR}
    )

target_compile_definitions(gor_common_setup INTERFACE ASIO_STANDALONE)

target_link_libraries(gor_common_setup INTERFACE Threads::Threads)

# customize binary
set_target_properties(gor_common_setup PROPERTIES RELEASE_POSTFIX "-${PROJECT_VERSION}")
set_target_properties(gor_common_setup PROPERTIES DEBUG_POSTFIX "-${PROJECT_VERSION}d")

# Create examples executable target
add_executable(stop1 stop1.cpp)
target_link_libraries(stop1 PRIVATE gor_common_setup)

add_executable(first first.cpp)
target_link_libraries(first PRIVATE gor_common_setup)

add_executable(easy easy.cpp)
target_link_libraries(easy PRIVATE gor_common_setup)

add_executable(hard1 hard1.cpp)
target_link_libraries(hard1 PRIVATE gor_common_setup)

add_executable(hard2 hard2.cpp)
target_link_libraries(hard2 PRIVATE gor_common_setup)

add_executable(over1 over1.cpp)
target_link_libraries(over1 PRIVATE gor_common_setup)

add_executable(over2 over2.cpp)
target_link_libraries(over2 PRIVATE gor_common_setup)

# asio official performance tests
add_executable(classic_client classic_client.cpp)
target_link_libraries(classic_client PRIVATE gor_common_setup)
add_executable(classic_server classic_server.cpp)
target_link_libraries(classic_server PRIVATE gor_common_setup)

# asio modified performance tests (coroutines usage)
add_executable(client client.cpp)
target_link_libraries(client PRIVATE gor_common_setup)
add_executable(server server.cpp)
target_link_libraries(server PRIVATE gor_common_setup)

# install
install(
    TARGETS
        stop1
        first
        easy
        hard1
        hard2
        over1
        over2
        classic_client
        classic_server
        client
        server
    RUNTIME DESTINATION .
)
