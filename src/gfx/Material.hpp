#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

// Forward declare your wrapper (adjust include/name if needed)
class ShaderProgram;

struct MaterialUniforms {
    GLint uMVP    = -1;
    GLint uFace   = -1;
    GLint uNode   = -1;
    GLint uRadius = -1;
    GLint uColor  = -1;

    bool valid() const {
        // only require what you actually use; all five are used in your current shader
        return uMVP != -1 && uFace != -1 && uNode != -1 && uRadius != -1 && uColor != -1;
    }
};

struct Material {
    ShaderProgram* shader = nullptr;
    MaterialUniforms loc{};

    // Values (set by App per frame / per draw)
    glm::mat4 mvp{1.0f};
    float radius = 1.0f;

    int face = 0;
    glm::vec3 node{0.0f};   // x,y,size
    glm::vec3 color{1.0f};

    // Call once after shader is linked
    void cache_uniforms();
};