cmake_minimum_required(VERSION 3.28)

project(Engine LANGUAGES CXX)

add_executable(app main.cpp)

target_compile_features(app PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(app PRIVATE /W4)
endif()
