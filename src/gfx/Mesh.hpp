#pragma once

#include <cstdint>

#include <glad/gl.h>

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    GLsizei indexCount = 0;

    static Mesh build_shared_grid(int N);
    void destroy();
};