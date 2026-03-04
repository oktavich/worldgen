#include "planet/Lod.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

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
    float patchBound = p.radius * theta * p.boundFactor + p.maxHeight;

    // Use distance to the patch's bounding sphere so refinement stays local
    // around the camera instead of refining most visible patches uniformly.
    float d = glm::length(camPos - c) - patchBound;
    d = std::max(d, p.dMin);

    float pixelsPerWorldUnit =
        (float(p.viewportH) / (2.0f * std::tan(p.fovY * 0.5f))) / d;

    float errorWorld = compute_error_world(n, p);
    float errPx = errorWorld * pixelsPerWorldUnit;

    // Focus detail in a camera-centered spherical ring, independent of cube face.
    const glm::vec3 focusDir = glm::normalize(camPos);
    const glm::vec3 patchDir = glm::normalize(c);
    const float cosA = glm::clamp(glm::dot(focusDir, patchDir), -1.0f, 1.0f);
    const float surfaceDist = std::acos(cosA) * p.radius;

    float focusT = 1.0f;
    if (p.focusOuterMeters > p.focusInnerMeters) {
        focusT = glm::clamp((surfaceDist - p.focusInnerMeters) /
                            (p.focusOuterMeters - p.focusInnerMeters), 0.0f, 1.0f);
    } else {
        focusT = (surfaceDist > p.focusOuterMeters) ? 1.0f : 0.0f;
    }
    const float focusScale = glm::mix(1.0f, p.focusOutsideScale, focusT);
    return errPx * focusScale;
}

static void collect_ops_rec(int face, QuadNode* n, const glm::vec3& camPos, const LodParams& p,
                            std::vector<LodOp>& outOps) {
    if (!n) return;

    glm::vec3 center = node_center_on_sphere(face, *n, p.radius);

    float theta = node_theta(*n);
    float patchBound = p.radius * theta * p.boundFactor + p.maxHeight;

    const bool visible = is_patch_visible_from_camera(camPos, center, p.radius, patchBound);

    // Beyond horizon: merge if possible, never split.
    if (!visible) {
        if (!n->is_leaf()) {
            outOps.push_back(LodOp{n, false, 1000.0f + float(n->level)});
        }
        return;
    }

    float errPx = compute_error_pixels(face, *n, camPos, p);

    if (n->is_leaf()) {
        if (n->level < p.maxLevel && errPx > p.targetPixels) {
            const float splitPriority = errPx / p.targetPixels;
            outOps.push_back(LodOp{n, true, splitPriority});
        }
        return;
    }

    if (errPx < p.targetPixels * p.mergeFactor) {
        const float margin = (p.targetPixels * p.mergeFactor) - errPx;
        outOps.push_back(LodOp{n, false, margin});
        return;
    }

    for (auto* c : n->child) collect_ops_rec(face, c, camPos, p, outOps);
}

void collect_lod_ops(int face, QuadNode* n, const glm::vec3& camPos, const LodParams& p,
                     std::vector<LodOp>& outOps) {
    collect_ops_rec(face, n, camPos, p, outOps);
}
