#include "planet/PatchBuilder.hpp"

#include <cmath>

#include <glm/glm.hpp>

static glm::vec3 cubeFacePoint(int face, float u, float v) {
  // u,v in [-1,1]
  if (face == 0) return glm::vec3(+1.0f, v,     -u); // +X
  if (face == 1) return glm::vec3(-1.0f, v,     +u); // -X
  if (face == 2) return glm::vec3(u,     +1.0f, -v); // +Y
  if (face == 3) return glm::vec3(u,     -1.0f, +v); // -Y
  if (face == 4) return glm::vec3(u,     v,     +1.0f); // +Z
  return            glm::vec3(-u,    v,     -1.0f); // -Z
}

namespace PatchBuilder {

void build_patch(int face, const QuadNode& node, int gridN, float radiusMeters,
                 const Terrain::HeightField& heightField, const PatchBuildParams& params,
                 std::vector<VertexPN>& outVerts) {
  outVerts.clear();
  outVerts.resize(size_t(gridN) * size_t(gridN) + size_t(gridN) * 4ull);

  auto write_vertex = [&](size_t index, float u01, float v01, float radiusOffset) {
    float u = u01 * 2.0f - 1.0f;
    float v = v01 * 2.0f - 1.0f;

    glm::vec3 cube = cubeFacePoint(face, u, v);
    glm::vec3 nrm = glm::normalize(cube);
    const float height = heightField.sample_height_m(nrm);
    glm::vec3 pos = nrm * (radiusMeters + height - radiusOffset);

    outVerts[index] = VertexPN{pos, nrm};
  };

  auto grid_index = [gridN](int x, int y) -> size_t {
    return size_t(y) * size_t(gridN) + size_t(x);
  };

  auto sample_displaced_position = [&](const glm::vec3& dir) -> glm::vec3 {
    glm::vec3 unitDir = glm::normalize(dir);
    float h = heightField.sample_height_m(unitDir);
    return unitDir * (radiusMeters + h);
  };

  for (int y = 0; y < gridN; ++y) {
    float ty = float(y) / float(gridN - 1);
    float v01 = node.y + ty * node.size;

    for (int x = 0; x < gridN; ++x) {
      float tx = float(x) / float(gridN - 1);
      float u01 = node.x + tx * node.size;
      write_vertex(size_t(y) * size_t(gridN) + size_t(x), u01, v01, 0.0f);
    }
  }

  // First pass: cheap local normals from already-displaced grid positions.
  for (int y = 0; y < gridN; ++y) {
    const int y0 = (y > 0) ? y - 1 : y;
    const int y1 = (y + 1 < gridN) ? y + 1 : y;

    for (int x = 0; x < gridN; ++x) {
      const int x0 = (x > 0) ? x - 1 : x;
      const int x1 = (x + 1 < gridN) ? x + 1 : x;

      const glm::vec3 posL = outVerts[grid_index(x0, y)].pos;
      const glm::vec3 posR = outVerts[grid_index(x1, y)].pos;
      const glm::vec3 posD = outVerts[grid_index(x, y0)].pos;
      const glm::vec3 posU = outVerts[grid_index(x, y1)].pos;

      glm::vec3 nrm = glm::normalize(glm::cross(posR - posL, posU - posD));
      const glm::vec3 dir = glm::normalize(outVerts[grid_index(x, y)].pos);

      if (glm::dot(nrm, nrm) < 1e-10f) {
        nrm = dir;
      }

      if (glm::dot(nrm, dir) < 0.0f) {
        nrm = -nrm;
      }

      outVerts[grid_index(x, y)].nrm = nrm;
    }
  }

  // Second pass: seam-safe world-space normals only on borders.
  auto set_border_normal = [&](int x, int y) {
    const glm::vec3 dir = glm::normalize(outVerts[grid_index(x, y)].pos);
    const glm::vec3 helper = (std::abs(dir.y) < 0.95f)
        ? glm::vec3(0.0f, 1.0f, 0.0f)
        : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 tangentA = glm::normalize(glm::cross(helper, dir));
    const glm::vec3 tangentB = glm::normalize(glm::cross(dir, tangentA));

    const float eps = std::max(5e-4f, node.size * (2.0f / float(gridN - 1)));
    const glm::vec3 pL = sample_displaced_position(dir - tangentA * eps);
    const glm::vec3 pR = sample_displaced_position(dir + tangentA * eps);
    const glm::vec3 pD = sample_displaced_position(dir - tangentB * eps);
    const glm::vec3 pU = sample_displaced_position(dir + tangentB * eps);

    glm::vec3 nrm = glm::normalize(glm::cross(pR - pL, pU - pD));
    if (glm::dot(nrm, nrm) < 1e-10f) nrm = dir;
    if (glm::dot(nrm, dir) < 0.0f) nrm = -nrm;
    outVerts[grid_index(x, y)].nrm = nrm;
  };

  for (int x = 0; x < gridN; ++x) {
    set_border_normal(x, 0);
    set_border_normal(x, gridN - 1);
  }
  for (int y = 1; y < gridN - 1; ++y) {
    set_border_normal(0, y);
    set_border_normal(gridN - 1, y);
  }

  const float skirtDepth = params.skirtDepthMeters;
  size_t skirtBase = size_t(gridN) * size_t(gridN);

  for (int x = 0; x < gridN; ++x) {
    float tx = float(x) / float(gridN - 1);
    write_vertex(skirtBase + size_t(x), node.x + tx * node.size, node.y + node.size, skirtDepth);
    outVerts[skirtBase + size_t(x)].nrm = outVerts[grid_index(x, gridN - 1)].nrm;
  }
  skirtBase += size_t(gridN);

  for (int y = 0; y < gridN; ++y) {
    float ty = float(y) / float(gridN - 1);
    write_vertex(skirtBase + size_t(y), node.x + node.size, node.y + ty * node.size, skirtDepth);
    outVerts[skirtBase + size_t(y)].nrm = outVerts[grid_index(gridN - 1, y)].nrm;
  }
  skirtBase += size_t(gridN);

  for (int x = 0; x < gridN; ++x) {
    float tx = float(x) / float(gridN - 1);
    write_vertex(skirtBase + size_t(x), node.x + tx * node.size, node.y, skirtDepth);
    outVerts[skirtBase + size_t(x)].nrm = outVerts[grid_index(x, 0)].nrm;
  }
  skirtBase += size_t(gridN);

  for (int y = 0; y < gridN; ++y) {
    float ty = float(y) / float(gridN - 1);
    write_vertex(skirtBase + size_t(y), node.x, node.y + ty * node.size, skirtDepth);
    outVerts[skirtBase + size_t(y)].nrm = outVerts[grid_index(0, y)].nrm;
  }
}

} // namespace PatchBuilder
