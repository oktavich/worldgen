#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "app/App.hpp"

static void glfw_error_callback(int code, const char* desc) {
    std::cerr << "GLFW error " << code << ": " << desc << "\n";
}

int main() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "worldgen - step0 LOD cubesphere (scale-aware)", nullptr, nullptr);
    if (!win) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to load OpenGL via GLAD\n";
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }

    // Create app and bind it to the window so callbacks can reach it.
    App app(win);
    glfwSetWindowUserPointer(win, &app);

    // Input callbacks forward into App (camera controls)
    glfwSetMouseButtonCallback(win, &App::glfw_mouse_button_cb);
    glfwSetCursorPosCallback(win, &App::glfw_cursor_pos_cb);
    glfwSetScrollCallback(win, &App::glfw_scroll_cb);

    if (!app.init()) {
        std::cerr << "App init failed\n";
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        app.update();
        app.render();

        glfwSwapBuffers(win);
    }

    // App destructor will clean up GL + CPU resources
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}