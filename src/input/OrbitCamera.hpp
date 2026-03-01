#pragma once

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

    bool dragging = false;
    double lastX = 0.0;
    double lastY = 0.0;

    float distance_from_center() const;
    glm::vec3 position() const;
    glm::mat4 view() const;

    // Input
    void on_mouse_button(GLFWwindow* win, int button, int action);
    void on_cursor_pos(double x, double y);
    void on_scroll(double yoff);
};