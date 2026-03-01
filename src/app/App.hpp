#pragma once

#include <array>
#include <imgui.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "gfx/Renderer.hpp"
#include "gfx/Material.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Mesh.hpp"
#include "input/OrbitCamera.hpp"
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
    Mesh m_grid{};
    Renderer m_renderer{};
    Material m_material{};
    bool m_wireframe = true;
    bool m_showUI = true;
    std::array<QuadNode*, 6> m_roots{};
      // FPS
    double m_fpsLastTime = 0.0;
    int    m_fpsFrames   = 0;
    float  m_fps         = 0.0f;

    LodParams m_lod{};
    float m_nearPlane = 0.1f;
    float m_farPlane  = 1000.0f;

    // Render constants
    int m_gridN = 33;

    std::array<glm::vec3, 6> m_faceColors{};



private:
    void compute_clip_planes(float altitudeMeters);
    void on_key(int key, int scancode, int action, int mods);
};