#pragma once

#include <array>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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

private:
    GLFWwindow* m_win = nullptr;

    OrbitCamera m_cam{};

    ShaderProgram m_prog{};
    Mesh m_grid{};

    std::array<QuadNode*, 6> m_roots{};

    LodParams m_lod{};
    float m_nearPlane = 0.1f;
    float m_farPlane  = 1000.0f;

    // Render constants
    int m_gridN = 33;

    std::array<glm::vec3, 6> m_faceColors{};

    // Cached uniform locations
    GLint m_uMVP    = -1;
    GLint m_uFace   = -1;
    GLint m_uNode   = -1;
    GLint m_uRadius = -1;
    GLint m_uColor  = -1;

private:
    void compute_clip_planes(float altitudeMeters);
};