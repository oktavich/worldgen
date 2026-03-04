#include "input/OrbitCamera.hpp"

#include <cmath>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

float OrbitCamera::distance_from_center() const {
    if (groundMode && groundRadius > 0.0f) return groundRadius;
    return planetRadius + altitude;
}

glm::vec3 OrbitCamera::position() const {
    if (groundMode) return groundDir * distance_from_center();

    float p = glm::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));
    float r = distance_from_center();

    glm::vec3 eye;
    eye.x = r * std::cos(p) * std::cos(yaw);
    eye.y = r * std::sin(p);
    eye.z = r * std::cos(p) * std::sin(yaw);
    return eye;
}

glm::mat4 OrbitCamera::view() const {
    if (!groundMode) {
        return glm::lookAt(position(), glm::vec3(0.0f), glm::vec3(0, 1, 0));
    }

    const glm::vec3 pos = position();
    const glm::vec3 up = glm::normalize(pos);
    glm::vec3 ref = (std::abs(up.y) < 0.95f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 east = glm::normalize(glm::cross(ref, up));
    glm::vec3 north = glm::normalize(glm::cross(up, east));

    glm::vec3 flatForward = glm::normalize(std::cos(yaw) * north + std::sin(yaw) * east);
    glm::vec3 forward = glm::normalize(std::cos(pitch) * flatForward + std::sin(pitch) * up);
    return glm::lookAt(pos, pos + forward * 50.0f, up);
}

void OrbitCamera::set_ground_mode(bool enabled, const std::function<float(const glm::vec3&)>& sampleHeight) {
    if (enabled == groundMode) return;
    groundMode = enabled;

    if (groundMode) {
        groundDir = glm::normalize(position());
        float h = sampleHeight(groundDir);
        groundRadius = planetRadius + h + eyeOffset + groundExtraHeight + minClearance;
        planarVelocity = glm::vec3(0.0f);
        altitude = groundRadius - planetRadius;
    } else {
        altitude = std::max(2.0f, distance_from_center() - planetRadius);
    }
}

void OrbitCamera::update_ground(GLFWwindow* win, float dt,
                                const std::function<float(const glm::vec3&)>& sampleHeight) {
    if (!groundMode || dt <= 0.0f) return;

    glm::vec3 pos = position();
    glm::vec3 up = glm::normalize(pos);
    glm::vec3 ref = (std::abs(up.y) < 0.95f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 east = glm::normalize(glm::cross(ref, up));
    glm::vec3 north = glm::normalize(glm::cross(up, east));

    glm::vec3 flatForward = glm::normalize(std::cos(yaw) * north + std::sin(yaw) * east);
    glm::vec3 right = glm::normalize(glm::cross(flatForward, up));

    float moveX = 0.0f;
    float moveY = 0.0f;
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) moveY += 1.0f;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) moveY -= 1.0f;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) moveX += 1.0f;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) moveX -= 1.0f;

    glm::vec3 moveDir = flatForward * moveY + right * moveX;
    if (glm::dot(moveDir, moveDir) > 1e-8f) moveDir = glm::normalize(moveDir);

    const bool sprint = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                        glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const float targetSpeed = walkSpeed * (sprint ? sprintMultiplier : 1.0f);
    const glm::vec3 targetVel = moveDir * targetSpeed;

    const float accelAlpha = 1.0f - std::exp(-accel * dt);
    planarVelocity = glm::mix(planarVelocity, targetVel, accelAlpha);
    if (glm::dot(moveDir, moveDir) < 1e-8f) {
        planarVelocity *= std::exp(-damping * dt);
    }

    glm::vec3 displacement = planarVelocity * dt;
    float radiusSafe = std::max(distance_from_center(), planetRadius + eyeOffset + minClearance);
    if (glm::dot(displacement, displacement) > 0.0f) {
        glm::vec3 moved = glm::normalize(groundDir + displacement / radiusSafe);
        groundDir = moved;
    }

    float h = sampleHeight(groundDir);
    float desiredRadius = planetRadius + h + eyeOffset + groundExtraHeight + minClearance;

    if (groundRadius <= 0.0f) groundRadius = desiredRadius;
    float snapAlpha = 1.0f - std::exp(-radialSnap * dt);
    groundRadius = glm::mix(groundRadius, desiredRadius, snapAlpha);
    groundRadius = std::max(groundRadius, desiredRadius);
    altitude = groundRadius - planetRadius;
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
    pitch = glm::clamp(pitch, glm::radians(-85.0f), glm::radians(85.0f));
}

void OrbitCamera::on_scroll(double yoff) {
    if (groundMode) {
        walkSpeed *= (yoff > 0.0) ? 1.08f : 0.92f;
        walkSpeed = glm::clamp(walkSpeed, 2.0f, 80.0f);
        return;
    }

    // Scroll up => closer to surface (lower altitude)
    altitude *= (yoff > 0.0) ? 0.90f : 1.10f;
    altitude = glm::clamp(altitude, 2.0f, 200000.0f); // 2m..200km
}
