#include <iostream>
#include <glm/glm.hpp>

int main() {
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);
    auto c = a + b;
    std::cout << "c = (" << c.x << ", " << c.y << ", " << c.z << ")\n";
}
