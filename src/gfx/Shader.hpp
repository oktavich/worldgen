#pragma once

#include <string>

#include <glad/gl.h>

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    bool build_from_sources(const char* vsSrc, const char* fsSrc);
    void destroy();

    void use() const;

    GLint uniform_location(const char* name) const;

    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;

private:
    static GLuint compile(GLenum type, const char* src);
};