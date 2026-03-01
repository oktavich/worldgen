#include "planet/Lod.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "planet/CubeSphere.hpp"
#include "planet/Quadtree.hpp"

// Conservative horizon test with patch bound.
// Keep if dot(C,P) > R^2 - R*patchBound
static bool is_patch_visible_from_camera(const glm::vec3& camPos,
                                         const glm::vec3& patchCenterOnSphere,
                                         float R,
                                         float patchBound) {
    return glm::dot(camPos, patchCenterOnSphere) > (R * R - R * patchBound);
}

static float node_theta(const QuadNode& n) {
    // cube face spans pi/2 radians; node spans (pi/2)*size
    return (std::numbers::pi_v<float> * 0.5f) * n.size;
}

float compute_error_world(const QuadNode& n, const LodParams& p) {
    // Face spans ~ pi/2 radians; node spans theta = (pi/2)*size.
    // A grid of (gridN-1) segments across => thetaSeg = theta/(gridN-1).
    // Curvature sagitta approx: error ~ (R/8) * thetaSeg^2
    float theta = node_theta(n);
    float thetaSeg = theta / float(p.gridN - 1);
    return (p.radius * 0.125f) * (thetaSeg * thetaSeg);
}

float compute_error_pixels(int face, const QuadNode& n, const glm::vec3& camPos, const LodParams& p) {
    glm::vec3 c = node_center_on_sphere(face, n, p.radius);

    float theta = node_theta(n);
    float patchBound = p.radius * theta * p.boundFactor;

    // Altitude-based distance (stable for space->ground transitions)
    float camR = glm::length(camPos);
    float altitude = camR - p.radius;
    altitude = std::max(altitude, 0.0f);

    // Center-distance variant (useful when far away)
    float centerDist = glm::length(camPos - c) - patchBound;

    // Blend: near ground rely on altitude; in space rely on centerDist
    float t = glm::clamp(altitude / (p.radius * 0.25f), 0.0f, 1.0f);
    float d = glm::mix(altitude, centerDist, t);

    d = std::max(d, p.dMin);

    float pixelsPerWorldUnit =
        (float(p.viewportH) / (2.0f * std::tan(p.fovY * 0.5f))) / d;

    float errorWorld = compute_error_world(n, p);
    return errorWorld * pixelsPerWorldUnit;
}

static void refine_or_merge(int face, QuadNode* n, const glm::vec3& camPos, const LodParams& p) {
    if (!n) return;

    glm::vec3 center = node_center_on_sphere(face, *n, p.radius);

    float theta = node_theta(*n);
    float patchBound = p.radius * theta * p.boundFactor;

    const bool visible = is_patch_visible_from_camera(camPos, center, p.radius, patchBound);

    // Beyond horizon: never split, aggressively collapse
    if (!visible) {
        if (!n->is_leaf()) merge_node(n);
        return;
    }

    float errPx = compute_error_pixels(face, *n, camPos, p);

    if (n->is_leaf()) {
        if (n->level < p.maxLevel && errPx > p.targetPixels) {
            split_node(n);
        }
        return;
    }

    if (errPx < p.targetPixels * p.mergeFactor) {
        merge_node(n);
        return;
    }

    for (auto* c : n->child) refine_or_merge(face, c, camPos, p);
}

void update_lod(int face, QuadNode* n, const glm::vec3& camPos, const LodParams& p) {
    refine_or_merge(face, n, camPos, p);
}