#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include <imgui.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "gfx/Renderer.hpp"
#include "gfx/Material.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Mesh.hpp"
#include "input/OrbitCamera.hpp"
#include "terrain/HeightField.hpp"
#include "planet/Quadtree.hpp"
#include "planet/Lod.hpp"

class App {
public:
    explicit App(GLFWwindow* window);
    ~App();

    bool init();
    void update();
    void render();

    // GLFW callback trampolines
    static void glfw_mouse_button_cb(GLFWwindow* win, int button, int action, int mods);
    static void glfw_cursor_pos_cb(GLFWwindow* win, double x, double y);
    static void glfw_scroll_cb(GLFWwindow* win, double xoff, double yoff);
    static void glfw_key_cb(GLFWwindow* win, int key, int scancode, int action, int mods);
private:
    GLFWwindow* m_win = nullptr;

    OrbitCamera m_cam{};

    ShaderProgram m_prog{};
    std::array<Mesh, 16> m_gridVariants{};
    std::unordered_map<uint64_t, Mesh> m_patchCache{};
    Terrain::HeightField m_heightField{};
    Renderer m_renderer{};
    Material m_material{};
    bool m_wireframe = true;
    bool m_showUI = true;
    bool m_useFaceColors = false;
    std::array<QuadNode*, 6> m_roots{};
      // FPS
    double m_fpsLastTime = 0.0;
    int    m_fpsFrames   = 0;
    float  m_fps         = 0.0f;
    double m_lastUpdateTime = 0.0;

    LodParams m_lod{};
    float m_nearPlane = 0.1f;
    float m_farPlane  = 1000.0f;

    // Render constants
    int m_gridN = 65;

    std::array<glm::vec3, 6> m_faceColors{};
    int m_cacheSize = 0;
    int m_cachePrunedLastFrame = 0;
    int m_maxLeafLevel = 0;
    int m_skirtEdgesThisFrame = 0;
    int m_lodSplitsLastFrame = 0;
    int m_lodMergesLastFrame = 0;
    float m_skirtDepthMeters = 8.0f;
    glm::vec3 m_planetGray{0.63f, 0.64f, 0.66f};

private:
    void compute_clip_planes(float altitudeMeters);
    void balance_lod_transitions();
    void prune_patch_cache();
    void on_key(int key, int scancode, int action, int mods);
};
