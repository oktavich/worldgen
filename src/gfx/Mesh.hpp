#pragma once

#include <cstdint>
#include <vector>

#include <glad/gl.h>

#include "gfx/Vertex.hpp"

struct Mesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ibo = 0;
  GLsizei indexCount = 0;

  // If false: this mesh references someone else's IBO (shared grid)
  bool ownsIbo = true;

  Mesh() = default;
  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  Mesh(Mesh&& other) noexcept;
  Mesh& operator=(Mesh&& other) noexcept;

  static Mesh build_shared_grid(int N, uint8_t skirtMask = 0);

  // Build a patch mesh (VertexPN VBO) that reuses the shared grid's IBO/indexCount.
  static Mesh build_patch_pn(const std::vector<VertexPN>& verts, const Mesh& sharedGrid);

  void destroy();
};
