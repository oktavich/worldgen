#include "gfx/Mesh.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace {

constexpr uint8_t kEdgeNorth = 1 << 0;
constexpr uint8_t kEdgeEast  = 1 << 1;
constexpr uint8_t kEdgeSouth = 1 << 2;
constexpr uint8_t kEdgeWest  = 1 << 3;

static std::vector<uint32_t> build_grid_indices(int N, uint8_t skirtMask) {
  std::vector<uint32_t> idx;
  idx.reserve(((N - 1) * (N - 1) + (N - 1) * 4) * 6);

  auto at = [N](int x, int y) -> uint32_t { return uint32_t(y * N + x); };

  for (int y = 0; y < N - 1; ++y) {
    for (int x = 0; x < N - 1; ++x) {
      uint32_t i0 = at(x, y);
      uint32_t i1 = at(x + 1, y);
      uint32_t i2 = at(x, y + 1);
      uint32_t i3 = at(x + 1, y + 1);

      idx.push_back(i0); idx.push_back(i2); idx.push_back(i1);
      idx.push_back(i1); idx.push_back(i2); idx.push_back(i3);
    }
  }

  uint32_t skirtBase = uint32_t(N * N);

  if (skirtMask & kEdgeNorth) {
    for (int x = 0; x < N - 1; ++x) {
      uint32_t e0 = at(x, N - 1);
      uint32_t e1 = at(x + 1, N - 1);
      uint32_t s0 = skirtBase + uint32_t(x);
      uint32_t s1 = skirtBase + uint32_t(x + 1);
      idx.push_back(e0); idx.push_back(s0); idx.push_back(e1);
      idx.push_back(e1); idx.push_back(s0); idx.push_back(s1);
    }
  }
  skirtBase += uint32_t(N);

  if (skirtMask & kEdgeEast) {
    for (int y = 0; y < N - 1; ++y) {
      uint32_t e0 = at(N - 1, y);
      uint32_t e1 = at(N - 1, y + 1);
      uint32_t s0 = skirtBase + uint32_t(y);
      uint32_t s1 = skirtBase + uint32_t(y + 1);
      idx.push_back(e0); idx.push_back(s0); idx.push_back(e1);
      idx.push_back(e1); idx.push_back(s0); idx.push_back(s1);
    }
  }
  skirtBase += uint32_t(N);

  if (skirtMask & kEdgeSouth) {
    for (int x = 0; x < N - 1; ++x) {
      uint32_t e0 = at(x, 0);
      uint32_t e1 = at(x + 1, 0);
      uint32_t s0 = skirtBase + uint32_t(x);
      uint32_t s1 = skirtBase + uint32_t(x + 1);
      idx.push_back(e0); idx.push_back(e1); idx.push_back(s0);
      idx.push_back(e1); idx.push_back(s1); idx.push_back(s0);
    }
  }
  skirtBase += uint32_t(N);

  if (skirtMask & kEdgeWest) {
    for (int y = 0; y < N - 1; ++y) {
      uint32_t e0 = at(0, y);
      uint32_t e1 = at(0, y + 1);
      uint32_t s0 = skirtBase + uint32_t(y);
      uint32_t s1 = skirtBase + uint32_t(y + 1);
      idx.push_back(e0); idx.push_back(e1); idx.push_back(s0);
      idx.push_back(e1); idx.push_back(s1); idx.push_back(s0);
    }
  }

  return idx;
}

} // namespace

Mesh::Mesh(Mesh&& other) noexcept {
  *this = std::move(other);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
  if (this == &other) return *this;

  // Free our current resources
  destroy();

  vao = other.vao;
  vbo = other.vbo;
  ibo = other.ibo;
  indexCount = other.indexCount;
  ownsIbo = other.ownsIbo;

  other.vao = 0;
  other.vbo = 0;
  other.ibo = 0;
  other.indexCount = 0;
  other.ownsIbo = true;

  return *this;
}

Mesh Mesh::build_shared_grid(int N, uint8_t skirtMask) {
  std::vector<glm::vec2> st;
  st.resize(size_t(N) * size_t(N));

  for (int y = 0; y < N; ++y) {
    float t = float(y) / float(N - 1);
    for (int x = 0; x < N; ++x) {
      float s = float(x) / float(N - 1);
      st[size_t(y) * size_t(N) + size_t(x)] = glm::vec2(s, t);
    }
  }

  std::vector<uint32_t> idx = build_grid_indices(N, skirtMask);

  Mesh m{};
  m.ownsIbo = true;

  glGenVertexArrays(1, &m.vao);
  glGenBuffers(1, &m.vbo);
  glGenBuffers(1, &m.ibo);

  glBindVertexArray(m.vao);

  glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
  glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(st.size() * sizeof(glm::vec2)), st.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(idx.size() * sizeof(uint32_t)), idx.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

  glBindVertexArray(0);

  m.indexCount = (GLsizei)idx.size();
  return m;
}

Mesh Mesh::build_patch_pn(const std::vector<VertexPN>& verts, const Mesh& sharedGrid) {
  Mesh m{};
  m.indexCount = sharedGrid.indexCount;
  m.ibo = sharedGrid.ibo;
  m.ownsIbo = false;

  glGenVertexArrays(1, &m.vao);
  glGenBuffers(1, &m.vbo);

  glBindVertexArray(m.vao);

  glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
  glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(verts.size() * sizeof(VertexPN)), verts.data(), GL_STATIC_DRAW);

  // IMPORTANT: share the grid's index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);

  // layout(location=0) vec3 aPos
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, pos));

  // layout(location=1) vec3 aNrm
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, nrm));

  glBindVertexArray(0);

  return m;
}

void Mesh::destroy() {
  if (vbo) glDeleteBuffers(1, &vbo);
  if (vao) glDeleteVertexArrays(1, &vao);

  if (ibo && ownsIbo) glDeleteBuffers(1, &ibo);

  vao = 0;
  vbo = 0;
  if (ownsIbo) ibo = 0;
  indexCount = 0;
  ownsIbo = true;
}
