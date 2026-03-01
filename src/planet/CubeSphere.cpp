#include "planet/CubeSphere.hpp"

glm::vec3 cube_face_point(int face, float u, float v) {
    // u,v in [-1,1]
    switch (face) {
        case 0: return glm::vec3(+1.0f, v,     -u);    // +X
        case 1: return glm::vec3(-1.0f, v,     +u);    // -X
        case 2: return glm::vec3(u,     +1.0f, -v);    // +Y
        case 3: return glm::vec3(u,     -1.0f, +v);    // -Y
        case 4: return glm::vec3(u,     v,     +1.0f); // +Z
        case 5: return glm::vec3(-u,    v,     -1.0f); // -Z
        default: return glm::vec3(0);
    }
}

glm::vec3 node_center_on_sphere(int face, const QuadNode& n, float R) {
    float xm = n.x + n.size * 0.5f;
    float ym = n.y + n.size * 0.5f;

    float u = xm * 2.0f - 1.0f;
    float v = ym * 2.0f - 1.0f;

    glm::vec3 p = cube_face_point(face, u, v);
    return glm::normalize(p) * R;
}