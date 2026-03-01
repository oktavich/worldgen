#include "input/OrbitCamera.hpp"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

float OrbitCamera::distance_from_center() const {
    return planetRadius + altitude;
}

glm::vec3 OrbitCamera::position() const {
    float p = glm::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));
    float r = distance_from_center();

    glm::vec3 eye;
    eye.x = r * std::cos(p) * std::cos(yaw);
    eye.y = r * std::sin(p);
    eye.z = r * std::cos(p) * std::sin(yaw);
    return eye;
}

glm::mat4 OrbitCamera::view() const {
    return glm::lookAt(position(), glm::vec3(0.0f), glm::vec3(0, 1, 0));
}

void OrbitCamera::on_mouse_button(GLFWwindow* win, int button, int action) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    if (action == GLFW_PRESS) {
        dragging = true;
        glfwGetCursorPos(win, &lastX, &lastY);
    } else if (action == GLFW_RELEASE) {
        dragging = false;
    }
}

void OrbitCamera::on_cursor_pos(double x, double y) {
    if (!dragging) return;

    double dx = x - lastX;
    double dy = y - lastY;
    lastX = x;
    lastY = y;

    const float sens = 0.005f;
    yaw   += float(dx) * sens;
    pitch += float(-dy) * sens;
}

void OrbitCamera::on_scroll(double yoff) {
    // Scroll up => closer to surface (lower altitude)
    altitude *= (yoff > 0.0) ? 0.90f : 1.10f;
    altitude = glm::clamp(altitude, 2.0f, 200000.0f); // 2m..200km
}