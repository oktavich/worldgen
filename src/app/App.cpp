#include "app/App.hpp"

#include <algorithm>
#include <cstdint>
#include <numbers>
#include <unordered_set>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp> // glm::clamp

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "planet/Scale.hpp"
#include "planet/Lod.hpp"
#include "planet/Quadtree.hpp"
#include "planet/CubeSphere.hpp"
#include "planet/PatchBuilder.hpp"
#include "gfx/Vertex.hpp"

static const char* kVertexShaderPNSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNrm;

uniform mat4 uMVP;

out vec3 vNrm;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vNrm = aNrm;
}
)";

static const char* kFragmentShaderPNSrc = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uLightDir;
uniform float uAmbient;
uniform float uDiffuseStrength;

in vec3 vNrm;

void main() {
    vec3 nrm = normalize(vNrm);
    vec3 lightDir = normalize(-uLightDir);
    float diffuse = max(dot(nrm, lightDir), 0.0) * uDiffuseStrength;
    float lighting = clamp(uAmbient + diffuse, 0.0, 1.5);
    FragColor = vec4(uColor * lighting, 1.0);
}
)";

// Conservative horizon test with patch bound.
// Keep if dot(C,P) > R^2 - R*patchBound
static bool is_patch_visible_from_camera(const glm::vec3& camPos,
                                         const glm::vec3& patchCenterOnSphere,
                                         float R,
                                         float patchBound) {
    return glm::dot(camPos, patchCenterOnSphere) > (R * R - R * patchBound);
}

static float node_theta(float nodeSize01) {
    // cube face spans pi/2 radians; node spans (pi/2)*size
    return (std::numbers::pi_v<float> * 0.5f) * nodeSize01;
}

static uint64_t patch_key(int face, const QuadNode& n, uint8_t variant = 0) {
    // x,y are multiples of size = 1 / (2^level)
    const int dim = 1 << n.level;
    const int ix = (int)glm::clamp(int(n.x * dim + 0.5f), 0, dim - 1);
    const int iy = (int)glm::clamp(int(n.y * dim + 0.5f), 0, dim - 1);

    // pack: face(3) | level(6) | variant(4) | ix(20) | iy(20)
    uint64_t k = 0;
    k |= (uint64_t(face) & 0x7ull) << 50;
    k |= (uint64_t(n.level) & 0x3Full) << 44;
    k |= (uint64_t(variant) & 0xFull) << 40;
    k |= (uint64_t(ix) & 0xFFFFFull) << 20;
    k |= (uint64_t(iy) & 0xFFFFFull);
    return k;
}

struct FaceUv {
    int face = 0;
    float u01 = 0.5f;
    float v01 = 0.5f;
};

static FaceUv cube_dir_to_face_uv(const glm::vec3& dir) {
    const glm::vec3 ad = glm::abs(dir);
    const float m = std::max(ad.x, std::max(ad.y, ad.z));
    glm::vec3 q = dir / m;
    FaceUv out{};

    const float ax = std::abs(q.x);
    const float ay = std::abs(q.y);
    const float az = std::abs(q.z);

    float u = 0.0f;
    float v = 0.0f;

    if (ax >= ay && ax >= az) {
        if (q.x >= 0.0f) {
            out.face = 0; // +X
            u = -q.z;
            v =  q.y;
        } else {
            out.face = 1; // -X
            u =  q.z;
            v =  q.y;
        }
    } else if (ay >= ax && ay >= az) {
        if (q.y >= 0.0f) {
            out.face = 2; // +Y
            u =  q.x;
            v = -q.z;
        } else {
            out.face = 3; // -Y
            u =  q.x;
            v =  q.z;
        }
    } else {
        if (q.z >= 0.0f) {
            out.face = 4; // +Z
            u =  q.x;
            v =  q.y;
        } else {
            out.face = 5; // -Z
            u = -q.x;
            v =  q.y;
        }
    }

    out.u01 = glm::clamp((u + 1.0f) * 0.5f, 0.0f, 1.0f);
    out.v01 = glm::clamp((v + 1.0f) * 0.5f, 0.0f, 1.0f);
    return out;
}

static FaceUv sample_neighbor_face_uv(int face, float u01, float v01) {
    float u = u01 * 2.0f - 1.0f;
    float v = v01 * 2.0f - 1.0f;
    glm::vec3 cube = cube_face_point(face, u, v);
    return cube_dir_to_face_uv(glm::normalize(cube));
}

static const QuadNode* find_neighbor_leaf_const(const std::array<QuadNode*, 6>& roots,
                                                int face, const QuadNode& n, uint8_t edgeMaskBit) {
    constexpr float kSampleOffset = 1e-5f;
    float u = n.x + n.size * 0.5f;
    float v = n.y + n.size * 0.5f;

    if (edgeMaskBit == PatchBuilder::EdgeNorth) {
        v = n.y + n.size + kSampleOffset;
    } else if (edgeMaskBit == PatchBuilder::EdgeEast) {
        u = n.x + n.size + kSampleOffset;
    } else if (edgeMaskBit == PatchBuilder::EdgeSouth) {
        v = n.y - kSampleOffset;
    } else {
        u = n.x - kSampleOffset;
    }

    FaceUv neighborUv = sample_neighbor_face_uv(face, u, v);
    if (!roots[neighborUv.face]) return nullptr;
    return find_leaf_at(roots[neighborUv.face], neighborUv.u01, neighborUv.v01);
}

static QuadNode* find_neighbor_leaf_mut(std::array<QuadNode*, 6>& roots,
                                        int face, const QuadNode& n, uint8_t edgeMaskBit) {
    constexpr float kSampleOffset = 1e-5f;
    float u = n.x + n.size * 0.5f;
    float v = n.y + n.size * 0.5f;

    if (edgeMaskBit == PatchBuilder::EdgeNorth) {
        v = n.y + n.size + kSampleOffset;
    } else if (edgeMaskBit == PatchBuilder::EdgeEast) {
        u = n.x + n.size + kSampleOffset;
    } else if (edgeMaskBit == PatchBuilder::EdgeSouth) {
        v = n.y - kSampleOffset;
    } else {
        u = n.x - kSampleOffset;
    }

    FaceUv neighborUv = sample_neighbor_face_uv(face, u, v);
    if (!roots[neighborUv.face]) return nullptr;
    return find_leaf_at_mut(roots[neighborUv.face], neighborUv.u01, neighborUv.v01);
}

static uint8_t compute_patch_skirt_mask(const std::array<QuadNode*, 6>& roots, int face, const QuadNode& n) {
    uint8_t mask = PatchBuilder::EdgeNone;

    const QuadNode* north = find_neighbor_leaf_const(roots, face, n, PatchBuilder::EdgeNorth);
    const QuadNode* east  = find_neighbor_leaf_const(roots, face, n, PatchBuilder::EdgeEast);
    const QuadNode* south = find_neighbor_leaf_const(roots, face, n, PatchBuilder::EdgeSouth);
    const QuadNode* west  = find_neighbor_leaf_const(roots, face, n, PatchBuilder::EdgeWest);

    if (!north || north->level < n.level) mask |= PatchBuilder::EdgeNorth;
    if (!east  || east->level  < n.level) mask |= PatchBuilder::EdgeEast;
    if (!south || south->level < n.level) mask |= PatchBuilder::EdgeSouth;
    if (!west  || west->level  < n.level) mask |= PatchBuilder::EdgeWest;

    return mask;
}

static int count_mask_bits(uint8_t value) {
    int count = 0;
    while (value) {
        count += int(value & 1u);
        value >>= 1u;
    }
    return count;
}

App::App(GLFWwindow* window) : m_win(window) {
    m_faceColors = {
        glm::vec3(1.0f, 0.3f, 0.3f),
        glm::vec3(0.3f, 1.0f, 0.3f),
        glm::vec3(0.3f, 0.6f, 1.0f),
        glm::vec3(1.0f, 1.0f, 0.3f),
        glm::vec3(1.0f, 0.3f, 1.0f),
        glm::vec3(0.3f, 1.0f, 1.0f),
    };
}

App::~App() {
    // ImGui shutdown (only if ImGui was actually initialized)
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    for (auto*& r : m_roots) {
        delete_subtree(r);
        r = nullptr;
    }

    // Destroy cached patch meshes (they do NOT own IBO, but they do own VAO/VBO)
    for (auto& kv : m_patchCache) {
        kv.second.destroy();
    }
    m_patchCache.clear();

    for (auto& grid : m_gridVariants) {
        grid.destroy();
    }
    m_prog.destroy();
}

bool App::init() {
    // Renderer owns baseline GL state now
    m_renderer.init();
    m_renderer.set_wireframe(m_wireframe);

    for (uint8_t mask = 0; mask < m_gridVariants.size(); ++mask) {
        m_gridVariants[mask] = Mesh::build_shared_grid(m_gridN, mask);
    }

    for (auto*& r : m_roots) r = new QuadNode{};

    m_lod.radius       = planet_radius_m();
    m_lod.gridN        = m_gridN;
    m_lod.maxLevel     = 10;
    m_lod.targetPixels = 2.5f;
    m_lod.mergeFactor  = 0.7f;
    m_lod.boundFactor  = 0.75f;
    m_lod.dMin         = 2.0f;
    m_lod.maxHeight    = m_heightField.max_height_m();
    m_lod.focusInnerMeters = 2500.0f;
    m_lod.focusOuterMeters = 12000.0f;
    m_lod.focusOutsideScale = 0.35f;
    m_lod.splitBudgetPerFrame = 96;
    m_lod.mergeBudgetPerFrame = 64;

    m_cam.planetRadius = m_lod.radius;
    m_cam.altitude     = 2000.0f;

    if (!m_prog.build_from_sources(kVertexShaderPNSrc, kFragmentShaderPNSrc)) {
        return false;
    }

    // Material owns shader + cached uniforms now
    m_material.shader = &m_prog;
    m_material.cache_uniforms();

    // ----- ImGui init -----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_win, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    // ----------------------

    return true;
}

void App::compute_clip_planes(float altitudeMeters) {
    float nearPlane = glm::clamp(altitudeMeters * 0.001f, 0.1f, 50.0f);
    float farPlane  = std::max(m_lod.radius * 10.0f, (m_lod.radius + altitudeMeters) * 4.0f);
    farPlane = std::min(farPlane, 2'000'000.0f);

    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

void App::update() {
    // -------- FPS COUNTER --------
    double now = glfwGetTime();
    if (m_fpsLastTime == 0.0) m_fpsLastTime = now;
    if (m_lastUpdateTime == 0.0) m_lastUpdateTime = now;

    m_fpsFrames++;
    double dt = now - m_fpsLastTime;
    float dtFrame = float(now - m_lastUpdateTime);
    m_lastUpdateTime = now;
    dtFrame = glm::clamp(dtFrame, 0.0f, 0.05f);

    if (dt >= 0.5) {  // update twice per second
        m_fps = float(double(m_fpsFrames) / dt);
        m_fpsFrames = 0;
        m_fpsLastTime = now;
    }
    // -----------------------------

    int w = 0, h = 0;
    glfwGetFramebufferSize(m_win, &w, &h);
    if (w <= 0 || h <= 0) return;

    if (m_cam.groundMode) {
        m_cam.update_ground(
            m_win, dtFrame,
            [this](const glm::vec3& unitDir) { return m_heightField.sample_height_m(unitDir); });
    }

    m_lod.viewportH = h;
    m_lod.fovY = glm::radians(60.0f);

    compute_clip_planes(m_cam.altitude);

    glm::vec3 camPos = m_cam.position();

    std::vector<LodOp> ops;
    ops.reserve(2048);
    for (int face = 0; face < 6; ++face) {
        collect_lod_ops(face, m_roots[face], camPos, m_lod, ops);
    }

    std::vector<LodOp> splitOps;
    std::vector<LodOp> mergeOps;
    splitOps.reserve(ops.size());
    mergeOps.reserve(ops.size());

    for (const LodOp& op : ops) {
        if (!op.node) continue;
        if (op.split) splitOps.push_back(op);
        else mergeOps.push_back(op);
    }

    std::sort(splitOps.begin(), splitOps.end(),
              [](const LodOp& a, const LodOp& b) { return a.priority > b.priority; });
    std::sort(mergeOps.begin(), mergeOps.end(),
              [](const LodOp& a, const LodOp& b) { return a.priority > b.priority; });

    int splitCount = 0;
    int mergeCount = 0;

    for (const LodOp& op : splitOps) {
        if (splitCount >= m_lod.splitBudgetPerFrame) break;
        if (!op.node->is_leaf()) continue;
        if (op.node->level >= m_lod.maxLevel) continue;
        split_node(op.node);
        ++splitCount;
    }

    for (const LodOp& op : mergeOps) {
        if (mergeCount >= m_lod.mergeBudgetPerFrame) break;
        if (op.node->is_leaf()) continue;
        merge_node(op.node);
        ++mergeCount;
    }

    m_lodSplitsLastFrame = splitCount;
    m_lodMergesLastFrame = mergeCount;

    balance_lod_transitions();
    prune_patch_cache();
}

void App::balance_lod_transitions() {
    constexpr int kMaxPasses = 8;
    constexpr int kMaxSplitsPerFrame = 48;
    int splitsThisFrame = 0;

    for (int pass = 0; pass < kMaxPasses; ++pass) {
        bool changed = false;

        for (int face = 0; face < 6 && !changed; ++face) {
            std::vector<QuadNode*> leaves;
            collect_leaves(m_roots[face], leaves);

            for (QuadNode* leaf : leaves) {
                static constexpr uint8_t kEdges[4] = {
                    PatchBuilder::EdgeNorth,
                    PatchBuilder::EdgeEast,
                    PatchBuilder::EdgeSouth,
                    PatchBuilder::EdgeWest
                };

                for (uint8_t edge : kEdges) {
                    QuadNode* neighbor = find_neighbor_leaf_mut(m_roots, face, *leaf, edge);
                    if (!neighbor) continue;
                    if (leaf->level <= neighbor->level + 1) continue;
                    if (neighbor->level >= m_lod.maxLevel) continue;
                    split_node(neighbor);
                    ++splitsThisFrame;
                    changed = true;
                    if (splitsThisFrame >= kMaxSplitsPerFrame) return;
                    break;
                }

                if (changed) break;
            }
        }

        if (!changed) break;
    }
}

void App::prune_patch_cache() {
    std::unordered_set<uint64_t> liveKeys;
    liveKeys.reserve(m_patchCache.size() + 64);

    int maxLeafLevel = 0;
    for (int face = 0; face < 6; ++face) {
        std::vector<QuadNode*> leaves;
        collect_leaves(m_roots[face], leaves);

        for (QuadNode* leaf : leaves) {
            const uint8_t skirtMask = compute_patch_skirt_mask(m_roots, face, *leaf);
            liveKeys.insert(patch_key(face, *leaf, skirtMask));
            maxLeafLevel = std::max(maxLeafLevel, leaf->level);
        }
    }

    int pruned = 0;
    for (auto it = m_patchCache.begin(); it != m_patchCache.end(); ) {
        if (liveKeys.contains(it->first)) {
            ++it;
            continue;
        }

        it->second.destroy();
        it = m_patchCache.erase(it);
        ++pruned;
    }

    m_cacheSize = static_cast<int>(m_patchCache.size());
    m_cachePrunedLastFrame = pruned;
    m_maxLeafLevel = maxLeafLevel;
}

void App::render() {
    int w = 0, h = 0;
    glfwGetFramebufferSize(m_win, &w, &h);
    if (w <= 0 || h <= 0) return;

    glm::mat4 proj = glm::perspective(m_lod.fovY, float(w) / float(h), m_nearPlane, m_farPlane);
    glm::mat4 view = m_cam.view();
    glm::mat4 mvp  = proj * view;

    m_renderer.begin_frame(w, h, {0.06f, 0.07f, 0.09f, 1.0f});
    m_renderer.set_wireframe(m_wireframe);

    m_material.mvp = mvp;
    m_material.lightDir = glm::normalize(m_material.lightDir);

    glm::vec3 camPos = m_cam.position();
    float R = m_lod.radius;

    int totalLeaves = 0;
    int faceLeaves[6] = {0, 0, 0, 0, 0, 0};
    int skirtEdgesThisFrame = 0;

    for (int face = 0; face < 6; ++face) {
        std::vector<QuadNode*> leaves;
        leaves.reserve(1024);
        collect_leaves(m_roots[face], leaves);

        faceLeaves[face] = (int)leaves.size();
        totalLeaves += faceLeaves[face];

        m_material.color = m_useFaceColors ? m_faceColors[face] : m_planetGray;

        for (QuadNode* n : leaves) {
            glm::vec3 center = node_center_on_sphere(face, *n, R);

            float theta      = node_theta(n->size);
            float patchBound = R * theta * m_lod.boundFactor + m_lod.maxHeight;

            if (!is_patch_visible_from_camera(camPos, center, R, patchBound)) continue;

            const uint8_t skirtMask = compute_patch_skirt_mask(m_roots, face, *n);
            const uint64_t key = patch_key(face, *n, skirtMask);
            skirtEdgesThisFrame += count_mask_bits(skirtMask);

            auto it = m_patchCache.find(key);
            if (it == m_patchCache.end()) {
                std::vector<VertexPN> verts;
                PatchBuilder::PatchBuildParams buildParams{};
                buildParams.skirtDepthMeters = std::max(m_skirtDepthMeters, n->size * m_lod.radius * 0.01f);
                PatchBuilder::build_patch(face, *n, m_gridN, m_lod.radius, m_heightField, buildParams, verts);

                Mesh patch = Mesh::build_patch_pn(verts, m_gridVariants[skirtMask]);
                it = m_patchCache.emplace(key, std::move(patch)).first;
            }

            m_renderer.draw(it->second, m_material);
        }
    } // ✅ closes face loop

    m_skirtEdgesThisFrame = skirtEdgesThisFrame;

    // --- ImGui overlay (F2 toggles) ---
    if (m_showUI) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Worldgen");
        ImGui::Text("FPS: %.1f", m_fps);
        ImGui::Text("Mode: %s", m_cam.groundMode ? "Ground" : "Space");
        ImGui::Text("Altitude: %.1f m", (double)m_cam.altitude);
        ImGui::Text("Altitude: %.2f km", (double)m_cam.altitude / 1000.0);
        ImGui::Text("Terrain range: %.0f .. %.0f m",
                    (double)m_heightField.min_height_m(),
                    (double)m_heightField.max_height_m());
        ImGui::Text("Leaf patches: %d", totalLeaves);
        ImGui::Text("Max leaf level: %d", m_maxLeafLevel);
        ImGui::Text("Patch cache: %d", m_cacheSize);
        ImGui::Text("Cache pruned: %d", m_cachePrunedLastFrame);
        ImGui::Text("Skirt edges: %d", m_skirtEdgesThisFrame);
        ImGui::Text("LOD ops S/M: %d / %d", m_lodSplitsLastFrame, m_lodMergesLastFrame);
        ImGui::Text("Face leaves: %d %d %d %d %d %d",
                    faceLeaves[0], faceLeaves[1], faceLeaves[2],
                    faceLeaves[3], faceLeaves[4], faceLeaves[5]);

        bool wf = m_wireframe;
        if (ImGui::Checkbox("Wireframe", &wf)) {
            m_wireframe = wf;
            m_renderer.set_wireframe(m_wireframe);
        }

        ImGui::Checkbox("Face colors", &m_useFaceColors);
        ImGui::ColorEdit3("Gray albedo", glm::value_ptr(m_planetGray));
        ImGui::SliderFloat3("Light dir", glm::value_ptr(m_material.lightDir), -1.0f, 1.0f);
        ImGui::SliderFloat("Ambient", &m_material.ambient, 0.0f, 1.0f);
        ImGui::SliderFloat("Diffuse", &m_material.diffuseStrength, 0.0f, 2.0f);
        ImGui::Separator();
        ImGui::SliderFloat("LOD focus inner m", &m_lod.focusInnerMeters, 200.0f, 20000.0f, "%.0f");
        ImGui::SliderFloat("LOD focus outer m", &m_lod.focusOuterMeters, 500.0f, 40000.0f, "%.0f");
        if (m_lod.focusOuterMeters < m_lod.focusInnerMeters + 10.0f) {
            m_lod.focusOuterMeters = m_lod.focusInnerMeters + 10.0f;
        }
        ImGui::SliderFloat("LOD outside scale", &m_lod.focusOutsideScale, 0.05f, 1.0f, "%.2f");
        ImGui::SliderInt("LOD split budget", &m_lod.splitBudgetPerFrame, 8, 256);
        ImGui::SliderInt("LOD merge budget", &m_lod.mergeBudgetPerFrame, 8, 256);
        ImGui::Separator();
        bool groundMode = m_cam.groundMode;
        if (ImGui::Checkbox("Camera on ground", &groundMode)) {
            m_cam.set_ground_mode(
                groundMode,
                [this](const glm::vec3& unitDir) { return m_heightField.sample_height_m(unitDir); });
        }
        ImGui::SliderFloat("Eye offset", &m_cam.eyeOffset, 1.2f, 3.0f, "%.2f m");
        ImGui::SliderFloat("Ground extra height", &m_cam.groundExtraHeight, 0.0f, 3000.0f, "%.1f m");
        ImGui::SliderFloat("Min clearance", &m_cam.minClearance, 0.05f, 1.0f, "%.2f m");
        ImGui::SliderFloat("Walk speed", &m_cam.walkSpeed, 2.0f, 80.0f, "%.1f m/s");
        ImGui::SliderFloat("Sprint x", &m_cam.sprintMultiplier, 1.0f, 4.0f, "%.2f");
        ImGui::SliderFloat("Accel", &m_cam.accel, 2.0f, 30.0f, "%.1f");
        ImGui::SliderFloat("Damping", &m_cam.damping, 1.0f, 20.0f, "%.1f");
        ImGui::SliderFloat("Ground snap", &m_cam.radialSnap, 2.0f, 40.0f, "%.1f");

        ImGui::Text("F1: wireframe");
        ImGui::Text("F2: UI toggle");
        ImGui::Text("F3: camera mode");
        ImGui::Text("WASD + Shift in ground mode");
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
} // ✅ closes render()

// -----------------------
// GLFW callback trampolines
// -----------------------
void App::glfw_mouse_button_cb(GLFWwindow* win, int button, int action, int /*mods*/) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(win));
    if (!app) return;
    app->m_cam.on_mouse_button(win, button, action);
}

void App::glfw_cursor_pos_cb(GLFWwindow* win, double x, double y) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(win));
    if (!app) return;
    app->m_cam.on_cursor_pos(x, y);
}

void App::glfw_scroll_cb(GLFWwindow* win, double /*xoff*/, double yoff) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(win));
    if (!app) return;
    app->m_cam.on_scroll(yoff);
}

void App::glfw_key_cb(GLFWwindow* win, int key, int scancode, int action, int mods) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(win));
    if (!app) return;
    app->on_key(key, scancode, action, mods);
}

void App::on_key(int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        m_wireframe = !m_wireframe;
        m_renderer.set_wireframe(m_wireframe);
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        m_showUI = !m_showUI;
    }

    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        m_cam.set_ground_mode(
            !m_cam.groundMode,
            [this](const glm::vec3& unitDir) { return m_heightField.sample_height_m(unitDir); });
    }
}
