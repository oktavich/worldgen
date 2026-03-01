#pragma once
#include <glm/glm.hpp>

struct Mesh;
struct Material;

class Renderer {
public:
    void init();
    void set_wireframe(bool enabled);
    void begin_frame(int width, int height, const glm::vec4& clearColor);
    void draw(const Mesh& mesh, const Material& material);
};