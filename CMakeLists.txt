cmake_minimum_required(VERSION 3.20)
project(worldgen LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(worldgen
  src/main.cpp
  extern/glad/src/gl.c
)

target_include_directories(worldgen PRIVATE
  extern/glad/include
)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(worldgen PRIVATE glfw OpenGL::GL)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(worldgen PRIVATE -Wall -Wextra -Wpedantic)
endif()
