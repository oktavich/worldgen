#include "gfx/Renderer.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include "gfx/Mesh.hpp"
#include "gfx/Material.hpp"
#include "gfx/Shader.hpp"

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);
}

void Renderer::set_wireframe(bool enabled) {
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

void Renderer::begin_frame(int width, int height, const glm::vec4& clearColor) {
    glViewport(0, 0, width, height);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw(const Mesh& mesh, const Material& mat) {
    if (!mat.shader) return;
    if (!mat.loc.valid()) return; // or assert/log if you prefer

    mat.shader->use();

    glUniformMatrix4fv(mat.loc.uMVP, 1, GL_FALSE, glm::value_ptr(mat.mvp));
    glUniform1i(mat.loc.uFace, mat.face);
    glUniform3fv(mat.loc.uNode, 1, glm::value_ptr(mat.node));
    glUniform1f(mat.loc.uRadius, mat.radius);
    glUniform3fv(mat.loc.uColor, 1, glm::value_ptr(mat.color));

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, (void*)0);
}