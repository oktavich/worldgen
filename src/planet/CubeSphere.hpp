#pragma once

#include <glm/glm.hpp>

#include "planet/Quadtree.hpp"

// CPU-side mapping helpers for LOD metric calculations.
// Face ids must match shader mapping.
glm::vec3 cube_face_point(int face, float u, float v);          // u,v in [-1,1]
glm::vec3 node_center_on_sphere(int face, const QuadNode& n, float R);