cmake_minimum_required(VERSION 3.28)

# ============================================================================
# Project
# ============================================================================
project(Engine LANGUAGES C CXX)

# ============================================================================
# Dependencies
# ============================================================================
include(FetchContent)

# --- GLFW -------------------------------------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.5.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glfw)

# --- glad -------------------------------------------------------------------
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

# ============================================================================
# App
# ============================================================================
add_executable(app main.cpp)

target_compile_features(app PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(app PRIVATE /W4)
endif()

target_link_libraries(app PRIVATE glfw glad)
