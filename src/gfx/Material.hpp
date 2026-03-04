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
    GLint uLightDir = -1;
    GLint uAmbient = -1;
    GLint uDiffuseStrength = -1;

    bool valid() const {
        return uMVP != -1 && uColor != -1 &&
               uLightDir != -1 && uAmbient != -1 && uDiffuseStrength != -1;
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
    glm::vec3 lightDir{glm::normalize(glm::vec3(-0.4f, -1.0f, -0.35f))};
    float ambient = 0.28f;
    float diffuseStrength = 0.9f;

    // Call once after shader is linked
    void cache_uniforms();
};
