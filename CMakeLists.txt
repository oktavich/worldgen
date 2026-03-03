cmake_minimum_required(VERSION 3.20)
project(worldgen LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dear ImGui sources (vendored as submodule)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui)
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(worldgen
  src/main.cpp
  src/gfx/Renderer.cpp
  src/gfx/Material.cpp
  src/app/App.cpp
  src/gfx/Shader.cpp
  src/gfx/Mesh.cpp
  src/input/OrbitCamera.cpp
  src/planet/CubeSphere.cpp
  src/planet/Quadtree.cpp
  src/planet/Lod.cpp
  src/planet/PatchBuilder.cpp

  ${IMGUI_SOURCES}

  extern/glad/src/gl.c
)

target_include_directories(worldgen PRIVATE
  src
  extern/glad/include

  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(worldgen PRIVATE glfw OpenGL::GL)

# ImGui backend needs to know which GL loader you're using.
# With GLAD, define this so imgui_impl_opengl3.cpp behaves correctly.
target_compile_definitions(worldgen PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(worldgen PRIVATE -Wall -Wextra -Wpedantic)
endif()