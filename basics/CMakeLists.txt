cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(BasicExamples
    VERSION 1.0
    DESCRIPTION "Basic coroutine examples"
    LANGUAGES CXX)

add_library(basics_common_setup INTERFACE)
target_compile_features(basics_common_setup INTERFACE cxx_std_20)
target_link_libraries(basics_common_setup INTERFACE common_stl_module)

# disable some warnings
if(MSVC)
    target_compile_options(basics_common_setup INTERFACE /W4 /WX /wd4834)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(basics_common_setup INTERFACE
            -Wno-sign-compare
            -Wno-unused-lambda-capture
        )
    endif()
    target_compile_definitions(basics_common_setup INTERFACE
        _WIN32_WINNT=0x0A00
        _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
    )
else()
    target_compile_options(basics_common_setup INTERFACE
        -Wall -Wextra -pedantic -Werror
        -Wno-implicit-fallthrough
    )
endif()

# customize binary
set_target_properties(basics_common_setup PROPERTIES
    RELEASE_POSTFIX "-${PROJECT_VERSION}"
    DEBUG_POSTFIX "-${PROJECT_VERSION}d"
)

# Create examples executable target
add_executable(await1 await/await1.cpp)
target_link_libraries(await1 PRIVATE basics_common_setup)

add_executable(await2 await/await2.cpp)
target_link_libraries(await2 PRIVATE basics_common_setup)

add_executable(await3 await/await3.cpp)
target_link_libraries(await3 PRIVATE basics_common_setup)

add_executable(await4 await/await4.cpp)
target_link_libraries(await4 PRIVATE basics_common_setup)

add_executable(future future/future.cpp)
target_link_libraries(future PRIVATE basics_common_setup)

add_executable(yield1 yield/yield1.cpp)
target_link_libraries(yield1 PRIVATE basics_common_setup)

add_executable(yield2 yield/yield2.cpp)
target_link_libraries(yield2 PRIVATE basics_common_setup)

add_executable(yield3 yield/yield3.cpp)
target_link_libraries(yield3 PRIVATE basics_common_setup)

# install
install(
    TARGETS
        await1
        await2
        await3
        await4
        future
        yield1
        yield2
        yield3
    RUNTIME DESTINATION .
)
