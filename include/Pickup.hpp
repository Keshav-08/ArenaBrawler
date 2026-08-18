#pragma once

#include "Common.hpp"
#include <array>
#include <vector>

enum class PickupKind { Health, PowerUp };

struct Pickup {
    Vector2 position{};
    float life = 0.0f;
    bool active = false;
    PickupKind kind = PickupKind::Health;
    PowerUpType powerUpType = PowerUpType::InstaKill; // meaningful only if kind == PowerUp
};

// Fixed-capacity pool of pickups dropped by dying enemies: mostly health
// orbs, occasionally one of the timed/instant power-ups (see
// cfg::kPowerUpDropShare). Same pattern as ParticleSystem/ProjectileManager:
// no per-frame heap allocation.
class PickupManager {
public:
    void SpawnHealth(Vector2 pos);
    void SpawnPowerUp(Vector2 pos, PowerUpType type);

    // Single entry point for "an enemy just died here" — rolls whether
    // anything drops at all (scaled by dropChanceMult), and if so, whether
    // it's health or a random power-up.
    void RollAndSpawnDrop(Vector2 pos);

    void Update(float dt);
    void Draw() const;

    // Deactivates any pickup within `radius` of `pos` and returns snapshots
    // of everything collected this call (empty if nothing was in range).
    std::vector<Pickup> CollectNear(Vector2 pos, float radius);

    float dropChanceMult = 1.0f; // difficulty-tuned, set once by Game

private:
    Pickup* AcquireSlot();

    std::array<Pickup, cfg::kMaxPickups> pool_{};
};
