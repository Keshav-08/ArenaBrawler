#pragma once

#include "Common.hpp"
#include <array>

struct Particle {
    Vector2 position{};
    Vector2 velocity{};
    Color color = WHITE;
    float life = 0.0f;
    float maxLife = 0.0f;
    float size = 2.0f;
    float drag = 1.0f; // velocity multiplier per second (1 = no drag)
    bool active = false;
};

// Fixed-size particle pool for hit sparks, death splatters, and muzzle
// flashes. No per-frame heap allocation.
class ParticleSystem {
public:
    void Update(float dt);
    void Draw() const;

    void SpawnBurst(Vector2 pos, int count, Color color, float speedMin, float speedMax,
                     float lifeMin, float lifeMax, float sizeMin = 2.0f, float sizeMax = 4.0f,
                     float drag = 0.9f);

    void SpawnMuzzleFlash(Vector2 pos, float angle, Color color);

private:
    Particle& Acquire();

    std::array<Particle, cfg::kMaxParticles> pool_{};
    size_t cursor_ = 0;
};
