cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(CoroutineExamples
    VERSION 1.0
    DESCRIPTION "C++ Coroutine examples from different sources"
    LANGUAGES CXX)

# include basic examples
add_subdirectory(basics)

# include Gor Nishanov's CppCon 2017 examples
add_subdirectory(Gor)
