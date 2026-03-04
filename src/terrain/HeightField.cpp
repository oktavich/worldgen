#include "terrain/HeightField.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <glm/common.hpp>

namespace Terrain {
namespace {

static float hash3d(int x, int y, int z, int seed) {
    uint32_t h = uint32_t(seed);
    h ^= uint32_t(x) * 0x27d4eb2du;
    h ^= uint32_t(y) * 0x165667b1u;
    h ^= uint32_t(z) * 0x9e3779b9u;
    h ^= h >> 15;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return float(h) / float(0xffffffffu);
}

static float smoothstep01(float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float value_noise_3d(const glm::vec3& p, int seed) {
    const int x0 = int(std::floor(p.x));
    const int y0 = int(std::floor(p.y));
    const int z0 = int(std::floor(p.z));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;
    const int z1 = z0 + 1;

    const float tx = smoothstep01(p.x - float(x0));
    const float ty = smoothstep01(p.y - float(y0));
    const float tz = smoothstep01(p.z - float(z0));

    const float c000 = hash3d(x0, y0, z0, seed);
    const float c100 = hash3d(x1, y0, z0, seed);
    const float c010 = hash3d(x0, y1, z0, seed);
    const float c110 = hash3d(x1, y1, z0, seed);
    const float c001 = hash3d(x0, y0, z1, seed);
    const float c101 = hash3d(x1, y0, z1, seed);
    const float c011 = hash3d(x0, y1, z1, seed);
    const float c111 = hash3d(x1, y1, z1, seed);

    const float x00 = glm::mix(c000, c100, tx);
    const float x10 = glm::mix(c010, c110, tx);
    const float x01 = glm::mix(c001, c101, tx);
    const float x11 = glm::mix(c011, c111, tx);
    const float y0v = glm::mix(x00, x10, ty);
    const float y1v = glm::mix(x01, x11, ty);

    return glm::mix(y0v, y1v, tz) * 2.0f - 1.0f;
}

static float fbm(const glm::vec3& p, int seed, int octaves) {
    float sum = 0.0f;
    float amp = 0.5f;
    float freq = 1.0f;
    float norm = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        sum += value_noise_3d(p * freq, seed + i * 101) * amp;
        norm += amp;
        freq *= 2.0f;
        amp *= 0.5f;
    }

    return (norm > 0.0f) ? (sum / norm) : 0.0f;
}

static float ridged_noise(const glm::vec3& p, int seed, int octaves) {
    float sum = 0.0f;
    float amp = 0.5f;
    float freq = 1.0f;
    float norm = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        float n = value_noise_3d(p * freq, seed + i * 131);
        n = 1.0f - std::abs(n);
        n *= n;
        sum += n * amp;
        norm += amp;
        freq *= 2.1f;
        amp *= 0.5f;
    }

    return (norm > 0.0f) ? (sum / norm) : 0.0f;
}

} // namespace

HeightField::HeightField(HeightFieldSettings settings) : m_settings(settings) {}

float HeightField::sample_height_m(const glm::vec3& unitSpherePos) const {
    const glm::vec3 p = unitSpherePos;

    const float continents = fbm(p * m_settings.continentFrequency * 8.0f, m_settings.seed, 5);
    const float continentalness = smoothstep01((continents + 0.28f) / 0.9f);

    const float mountains = ridged_noise(p * m_settings.mountainFrequency * 8.0f,
                                         m_settings.seed + 1000, 4);
    const float mountainMask = smoothstep01((continentalness - 0.35f) / 0.55f);
    const float peakShape = std::pow(glm::clamp(mountains, 0.0f, 1.0f), 3.5f);

    const float detail = fbm(p * m_settings.detailFrequency * 8.0f,
                             m_settings.seed + 2000, 3);
    const float detailMask = glm::mix(0.15f, 1.0f, mountainMask);

    float height = m_settings.seaLevelMeters;
    height += continentalness * m_settings.continentHeightMeters;
    height += peakShape * mountainMask * m_settings.mountainHeightMeters;
    height += detail * m_settings.detailHeightMeters * detailMask;

    return std::clamp(height, m_minHeightMeters, m_maxHeightMeters);
}

} // namespace Terrain
