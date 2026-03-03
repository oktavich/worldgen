#include "planet/PatchBuilder.hpp"

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
                 std::vector<VertexPN>& outVerts) {
  outVerts.clear();
  outVerts.resize(size_t(gridN) * size_t(gridN));

  for (int y = 0; y < gridN; ++y) {
    float ty = float(y) / float(gridN - 1);
    float v01 = node.y + ty * node.size;

    for (int x = 0; x < gridN; ++x) {
      float tx = float(x) / float(gridN - 1);
      float u01 = node.x + tx * node.size;

      float u = u01 * 2.0f - 1.0f;
      float v = v01 * 2.0f - 1.0f;

      glm::vec3 cube = cubeFacePoint(face, u, v);
      glm::vec3 nrm = glm::normalize(cube);
      glm::vec3 pos = nrm * radiusMeters;

      outVerts[size_t(y) * size_t(gridN) + size_t(x)] = VertexPN{pos, nrm};
    }
  }
}

} // namespace PatchBuilder