#include "app/App.hpp"

#include <algorithm>
#include <numbers>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "planet/Scale.hpp"
#include "planet/Lod.hpp"
#include "planet/Quadtree.hpp"
#include "planet/CubeSphere.hpp"

static const char* kVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aST;   // [0,1]x[0,1] grid

uniform mat4  uMVP;
uniform int   uFace;   // 0..5
uniform vec3  uNode;   // x,y,size in [0,1]
uniform float uRadius; // meters

vec3 cubeFacePoint(int face, float u, float v) {
    // u,v in [-1,1]
    if (face == 0) return vec3(+1.0, v,     -u);    // +X
    if (face == 1) return vec3(-1.0, v,     +u);    // -X
    if (face == 2) return vec3(u,     +1.0, -v);    // +Y
    if (face == 3) return vec3(u,     -1.0, +v);    // -Y
    if (face == 4) return vec3(u,     v,     +1.0); // +Z
    return            vec3(-u,    v,     -1.0);      // -Z
}

void main() {
    vec2 uv01 = uNode.xy + aST * uNode.z;   // [0,1] within node
    float u = uv01.x * 2.0 - 1.0;           // [-1,1]
    float v = uv01.y * 2.0 - 1.0;

    vec3 cube = cubeFacePoint(uFace, u, v);
    vec3 sphere = normalize(cube) * uRadius;

    gl_Position = uMVP * vec4(sphere, 1.0);
}
)";

static const char* kFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() { FragColor = vec4(uColor, 1.0); }
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
    m_grid.destroy();
    m_prog.destroy();
}

bool App::init() {
    // Renderer owns baseline GL state now
    m_renderer.init();
    m_renderer.set_wireframe(m_wireframe);

    m_grid = Mesh::build_shared_grid(m_gridN);

    for (auto*& r : m_roots) r = new QuadNode{};

    m_lod.radius       = planet_radius_m();
    m_lod.gridN        = m_gridN;
    m_lod.maxLevel     = 10;
    m_lod.targetPixels = 2.5f;
    m_lod.mergeFactor  = 0.7f;
    m_lod.boundFactor  = 0.75f;
    m_lod.dMin         = 2.0f;

    m_cam.planetRadius = m_lod.radius;
    m_cam.altitude     = 2000.0f;

    if (!m_prog.build_from_sources(kVertexShaderSrc, kFragmentShaderSrc)) {
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

    m_fpsFrames++;
    double dt = now - m_fpsLastTime;

    if (dt >= 0.5) {  // update twice per second
        m_fps = float(double(m_fpsFrames) / dt);
        m_fpsFrames = 0;
        m_fpsLastTime = now;
    }
    // -----------------------------

    int w = 0, h = 0;
    glfwGetFramebufferSize(m_win, &w, &h);
    if (w <= 0 || h <= 0) return;

    m_lod.viewportH = h;
    m_lod.fovY = glm::radians(60.0f);

    compute_clip_planes(m_cam.altitude);

    glm::vec3 camPos = m_cam.position();

    for (int face = 0; face < 6; ++face) {
        update_lod(face, m_roots[face], camPos, m_lod);
    }
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

    m_material.mvp    = mvp;
    m_material.radius = m_lod.radius;

    glm::vec3 camPos = m_cam.position();
    float R = m_lod.radius;

    int totalLeaves = 0;
    int faceLeaves[6] = {0, 0, 0, 0, 0, 0};

    for (int face = 0; face < 6; ++face) {
        std::vector<QuadNode*> leaves;
        leaves.reserve(1024);
        collect_leaves(m_roots[face], leaves);

        faceLeaves[face] = (int)leaves.size();
        totalLeaves += faceLeaves[face];

        m_material.face  = face;
        m_material.color = m_faceColors[face];

        for (QuadNode* n : leaves) {
            glm::vec3 center = node_center_on_sphere(face, *n, R);

            float theta      = node_theta(n->size);
            float patchBound = R * theta * m_lod.boundFactor;

            if (!is_patch_visible_from_camera(camPos, center, R, patchBound)) continue;

            m_material.node = glm::vec3(n->x, n->y, n->size);
            m_renderer.draw(m_grid, m_material);
        }
    }

    // --- ImGui overlay (F2 toggles) ---
    if (m_showUI) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Worldgen");
        ImGui::Text("FPS: %.1f", m_fps);
        ImGui::Text("Altitude: %.1f m", (double)m_cam.altitude);
        ImGui::Text("Altitude: %.2f km", (double)m_cam.altitude / 1000.0);
        ImGui::Text("Leaf patches: %d", totalLeaves);
        ImGui::Text("Face leaves: %d %d %d %d %d %d",
                    faceLeaves[0], faceLeaves[1], faceLeaves[2],
                    faceLeaves[3], faceLeaves[4], faceLeaves[5]);
        

        bool wf = m_wireframe;
        if (ImGui::Checkbox("Wireframe", &wf)) {
            m_wireframe = wf;
            m_renderer.set_wireframe(m_wireframe);
        }

        ImGui::Text("F1: wireframe");
        ImGui::Text("F2: UI toggle");
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

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
}