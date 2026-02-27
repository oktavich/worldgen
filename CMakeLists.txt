cmake_minimum_required(VERSION 3.20)
project(worldgen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(worldgen src/main.cpp)

# GLM is header-only, but this is the nice modern way:
find_package(glm CONFIG QUIET)
if(glm_FOUND)
  target_link_libraries(worldgen PRIVATE glm::glm)
else()
  message(STATUS "glm package config not found; using include path fallback")
  target_include_directories(worldgen PRIVATE /usr/include)
endif()
