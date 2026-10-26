cmake_minimum_required(VERSION 3.28)

project(Engine LANGUAGES CXX)

add_executable(sandbox main.cpp)

target_compile_features(sandbox PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(sandbox PRIVATE /W4)
endif()