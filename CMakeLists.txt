cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

option(USE_CXX_MODULES "Use C++20 modules" OFF)

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
