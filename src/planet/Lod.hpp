#pragma once

#include <glm/glm.hpp>

#include "planet/Quadtree.hpp"

struct LodParams {
    float radius = 1.0f;      // meters
    int gridN = 33;           // vertices per side
    int maxLevel = 10;

    float fovY = glm::radians(60.0f);
    int viewportH = 720;

    float targetPixels = 2.5f; // split if errorPixels > targetPixels
    float mergeFactor  = 0.7f; // merge if errorPixels < targetPixels*mergeFactor

    float boundFactor = 0.75f; // bounding radius proxy factor
    float dMin = 2.0f;         // meters: clamp to prevent LOD blowup near surface
};

float compute_error_world(const QuadNode& n, const LodParams& p);
float compute_error_pixels(int face, const QuadNode& n, const glm::vec3& camPos, const LodParams& p);

void update_lod(int face, QuadNode* n, const glm::vec3& camPos, const LodParams& p);