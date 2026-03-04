#pragma once
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "gfx/Vertex.hpp"
#include "planet/Quadtree.hpp"
#include "terrain/HeightField.hpp"

namespace PatchBuilder {
enum EdgeMask : uint8_t {
  EdgeNone  = 0,
  EdgeNorth = 1 << 0,
  EdgeEast  = 1 << 1,
  EdgeSouth = 1 << 2,
  EdgeWest  = 1 << 3,
};

struct PatchBuildParams {
  uint8_t skirtMask = EdgeNone;
  float skirtDepthMeters = 0.0f;
};

void build_patch(int face, const QuadNode& node, int gridN, float radiusMeters,
                 const Terrain::HeightField& heightField, const PatchBuildParams& params,
                 std::vector<VertexPN>& outVerts);
}
