#include "gfx/Material.hpp"
#include "gfx/Shader.hpp" // your ShaderProgram wrapper

void Material::cache_uniforms() {
    if (!shader) return;

    // Cache once. If you ever hot-reload/relink the shader, call again.
    loc.uMVP    = shader->uniform_location("uMVP");
    loc.uFace   = shader->uniform_location("uFace");
    loc.uNode   = shader->uniform_location("uNode");
    loc.uRadius = shader->uniform_location("uRadius");
    loc.uColor  = shader->uniform_location("uColor");
}