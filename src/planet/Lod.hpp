#pragma once

#include <vector>

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
    float maxHeight = 0.0f;    // meters: terrain displacement budget for bounds

    // Camera-centric focus ring on the sphere surface (meters).
    // Inside inner radius gets full detail; outside outer radius gets scaled-down error.
    float focusInnerMeters = 2500.0f;
    float focusOuterMeters = 12000.0f;
    float focusOutsideScale = 0.35f;

    // Budgeted LOD ops per frame.
    int splitBudgetPerFrame = 96;
    int mergeBudgetPerFrame = 64;
};

struct LodOp {
    QuadNode* node = nullptr;
    bool split = false;
    float priority = 0.0f;
};

float compute_error_world(const QuadNode& n, const LodParams& p);
float compute_error_pixels(int face, const QuadNode& n, const glm::vec3& camPos, const LodParams& p);
void collect_lod_ops(int face, QuadNode* n, const glm::vec3& camPos, const LodParams& p,
                     std::vector<LodOp>& outOps);
