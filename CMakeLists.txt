cmake_minimum_required(VERSION 3.14...4.3 FATAL_ERROR)

option(USE_CXX_MODULES "Use C++20 modules" OFF)

if(USE_CXX_MODULES AND UNIX AND (NOT CMAKE_EXPERIMENTAL_CXX_STD AND NOT CMAKE_CXX_MODULE_STD))
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30" AND CMAKE_VERSION VERSION_LESS_EQUAL "4.0")
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0" AND CMAKE_VERSION VERSION_LESS "4.0.3")
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0.3" AND CMAKE_VERSION VERSION_LESS "4.3")
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.3" AND CMAKE_VERSION VERSION_LESS "4.4")
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4" AND CMAKE_VERSION VERSION_LESS "4.5")
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
    endif()
endif()

project(CoroutineExamples
    VERSION 1.0
    DESCRIPTION "C++ Coroutine examples from different sources"
    LANGUAGES CXX)

# Share the same STL BMI on all the examples (speed up build)
if (USE_CXX_MODULES)
    add_library(common_stl_module STATIC)
    target_sources(common_stl_module
        PUBLIC FILE_SET modules TYPE CXX_MODULES
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/std_proxy.ixx
    )
    target_compile_definitions(common_stl_module PUBLIC USE_CXX_MODULES)
    target_compile_features(common_stl_module PUBLIC cxx_std_20)
    if (UNIX)
        # Pull in the standard library module (std / std.compat).
        set_target_properties(common_stl_module PROPERTIES CXX_MODULE_STD ON)
    endif()
else()
    add_library(common_stl_module INTERFACE)
    target_compile_features(common_stl_module INTERFACE cxx_std_20)
endif()

# include basic examples
add_subdirectory(basics)

# include Gor Nishanov's CppCon 2017 examples
add_subdirectory(Gor)

# include winrt examples if apply
if (WIN32)
    add_subdirectory(winrt)
endif()
