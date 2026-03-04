#pragma once

#include <glm/vec3.hpp>

namespace Terrain {

struct HeightFieldSettings {
    int seed = 1337;
    float seaLevelMeters = -120.0f;
    float continentFrequency = 0.85f;
    float mountainFrequency = 3.0f;
    float detailFrequency = 12.0f;
    float continentHeightMeters = 170.0f;
    float mountainHeightMeters = 130.0f;
    float detailHeightMeters = 18.0f;
};

class HeightField {
public:
    explicit HeightField(HeightFieldSettings settings = {});

    float sample_height_m(const glm::vec3& unitSpherePos) const;
    float min_height_m() const { return m_minHeightMeters; }
    float max_height_m() const { return m_maxHeightMeters; }

private:
    HeightFieldSettings m_settings{};
    float m_minHeightMeters = -200.0f;
    float m_maxHeightMeters = 300.0f;
};

} // namespace Terrain
