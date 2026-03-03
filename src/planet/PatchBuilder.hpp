#pragma once
#include <vector>

#include <glm/glm.hpp>

#include "gfx/Vertex.hpp"
#include "planet/Quadtree.hpp"

namespace PatchBuilder {
  void build_patch(int face, const QuadNode& node, int gridN, float radiusMeters,
                   std::vector<VertexPN>& outVerts);
}