#pragma once

#include <functional>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

struct OrbitCamera {
    float yaw   = glm::radians(45.0f);
    float pitch = glm::radians(-20.0f);

    // meters above surface
    float altitude = 2000.0f;

    // meters
    float planetRadius = 1.0f;

    bool groundMode = false;
    float eyeOffset = 2.0f;          // camera eye height above terrain
    float groundExtraHeight = 0.0f;  // optional extra hover above eye level (meters)
    float minClearance = 0.2f;       // collision constraint vs terrain
    float walkSpeed = 16.0f;         // m/s
    float sprintMultiplier = 2.4f;
    float accel = 12.0f;
    float damping = 7.0f;
    float radialSnap = 18.0f;        // higher = faster ground lock

    bool dragging = false;
    double lastX = 0.0;
    double lastY = 0.0;

    glm::vec3 groundDir{glm::normalize(glm::vec3(1.0f, 0.2f, 0.3f))};
    glm::vec3 planarVelocity{0.0f};
    float groundRadius = 0.0f;

    float distance_from_center() const;
    glm::vec3 position() const;
    glm::mat4 view() const;
    void set_ground_mode(bool enabled, const std::function<float(const glm::vec3&)>& sampleHeight);
    void update_ground(GLFWwindow* win, float dt, const std::function<float(const glm::vec3&)>& sampleHeight);

    // Input
    void on_mouse_button(GLFWwindow* win, int button, int action);
    void on_cursor_pos(double x, double y);
    void on_scroll(double yoff);
};
