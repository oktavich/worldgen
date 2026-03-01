#include "gfx/Mesh.hpp"

#include <vector>

#include <glm/glm.hpp>

static std::vector<uint32_t> build_grid_indices(int N) {
    std::vector<uint32_t> idx;
    idx.reserve((N - 1) * (N - 1) * 6);

    auto at = [N](int x, int y) -> uint32_t { return uint32_t(y * N + x); };

    for (int y = 0; y < N - 1; ++y) {
        for (int x = 0; x < N - 1; ++x) {
            uint32_t i0 = at(x, y);
            uint32_t i1 = at(x + 1, y);
            uint32_t i2 = at(x, y + 1);
            uint32_t i3 = at(x + 1, y + 1);

            idx.push_back(i0); idx.push_back(i2); idx.push_back(i1);
            idx.push_back(i1); idx.push_back(i2); idx.push_back(i3);
        }
    }
    return idx;
}

Mesh Mesh::build_shared_grid(int N) {
    std::vector<glm::vec2> st;
    st.resize(size_t(N) * size_t(N));

    for (int y = 0; y < N; ++y) {
        float t = float(y) / float(N - 1);
        for (int x = 0; x < N; ++x) {
            float s = float(x) / float(N - 1);
            st[size_t(y) * size_t(N) + size_t(x)] = glm::vec2(s, t);
        }
    }

    std::vector<uint32_t> idx = build_grid_indices(N);

    Mesh m{};
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ibo);

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(st.size() * sizeof(glm::vec2)), st.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(idx.size() * sizeof(uint32_t)), idx.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glBindVertexArray(0);

    m.indexCount = (GLsizei)idx.size();
    return m;
}

void Mesh::destroy() {
    if (ibo) glDeleteBuffers(1, &ibo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = ibo = 0;
    indexCount = 0;
}