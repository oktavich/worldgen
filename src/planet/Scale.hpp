#pragma once

// 1 unit = 1 meter
static constexpr float kCircumferenceMeters = 60000.0f; // ~60 km
static constexpr float kPi = 3.14159265358979323846f;

inline float planet_radius_m() {
    return kCircumferenceMeters / (2.0f * kPi); // ~9549m
}