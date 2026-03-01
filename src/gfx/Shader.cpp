#include "gfx/Shader.hpp"

#include <iostream>

ShaderProgram::~ShaderProgram() { destroy(); }

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept {
    m_id = other.m_id;
    other.m_id = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this == &other) return *this;
    destroy();
    m_id = other.m_id;
    other.m_id = 0;
    return *this;
}

GLuint ShaderProgram::compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(size_t(len), '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::cerr << "Shader compile failed:\n" << log << "\n";
    }
    return s;
}

bool ShaderProgram::build_from_sources(const char* vsSrc, const char* fsSrc) {
    destroy();

    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);

    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(size_t(len), '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::cerr << "Program link failed:\n" << log << "\n";
        glDeleteProgram(p);
        return false;
    }

    m_id = p;
    return true;
}

void ShaderProgram::destroy() {
    if (m_id) {
        glDeleteProgram(m_id);
        m_id = 0;
    }
}

void ShaderProgram::use() const { glUseProgram(m_id); }

GLint ShaderProgram::uniform_location(const char* name) const {
    return glGetUniformLocation(m_id, name);
}