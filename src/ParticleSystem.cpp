#include "ParticleSystem.hpp"

Particle& ParticleSystem::Acquire() {
    // Ring-buffer allocation: reuses the oldest slot once the pool is full,
    // which is visually fine for short-lived spark/splatter effects.
    Particle& p = pool_[cursor_];
    cursor_ = (cursor_ + 1) % pool_.size();
    return p;
}

void ParticleSystem::SpawnBurst(Vector2 pos, int count, Color color, float speedMin, float speedMax,
                                 float lifeMin, float lifeMax, float sizeMin, float sizeMax, float drag) {
    for (int i = 0; i < count; ++i) {
        Particle& p = Acquire();
        float angle = mathutil::RandomFloat(0.0f, 2.0f * PI);
        float speed = mathutil::RandomFloat(speedMin, speedMax);
        p.position = pos;
        p.velocity = mathutil::FromAngle(angle, speed);
        p.color = color;
        p.maxLife = mathutil::RandomFloat(lifeMin, lifeMax);
        p.life = p.maxLife;
        p.size = mathutil::RandomFloat(sizeMin, sizeMax);
        p.drag = drag;
        p.active = true;
    }
}

void ParticleSystem::SpawnMuzzleFlash(Vector2 pos, float angle, Color color) {
    for (int i = 0; i < 6; ++i) {
        Particle& p = Acquire();
        float spread = angle + mathutil::RandomFloat(-0.35f, 0.35f);
        float speed = mathutil::RandomFloat(180.0f, 420.0f);
        p.position = pos;
        p.velocity = mathutil::FromAngle(spread, speed);
        p.color = color;
        p.maxLife = mathutil::RandomFloat(0.06f, 0.14f);
        p.life = p.maxLife;
        p.size = mathutil::RandomFloat(2.0f, 4.0f);
        p.drag = 0.8f;
        p.active = true;
    }
}

void ParticleSystem::Update(float dt) {
    for (auto& p : pool_) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }
        p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
        float dragFactor = std::pow(p.drag, dt * 60.0f);
        p.velocity = Vector2Scale(p.velocity, dragFactor);
    }
}

void ParticleSystem::Draw() const {
    for (const auto& p : pool_) {
        if (!p.active) continue;
        float t = mathutil::Clamp01(p.life / p.maxLife);
        Color c = ColorAlpha(p.color, t);
        DrawCircleV(p.position, p.size * t + 0.5f, c);
    }
}
