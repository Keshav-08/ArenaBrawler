#pragma once

#include "Common.hpp"
#include <array>
#include <vector>

enum class PickupKind { Health, PowerUp, WeaponLoot, Ammo, Armor };

struct Pickup {
    Vector2 position{};
    float life = 0.0f;
    bool active = false;
    PickupKind kind = PickupKind::Health;
    PowerUpType powerUpType = PowerUpType::InstaKill;       // meaningful only if kind == PowerUp
    WeaponType weaponLootType = WeaponType::ChurroBlaster;   // meaningful only if kind == WeaponLoot
    float armorReduction = 0.0f;                             // meaningful only if kind == Armor
};

// Fixed-capacity pool of pickups: health orbs and power-ups dropped by dying
// enemies, plus itemization loot (ground weapons placed per-room in
// Level.cpp, and ammo/armor mixed into the kill-drop pool). Same pattern as
// ParticleSystem/ProjectileManager: no per-frame heap allocation.
class PickupManager {
public:
    void SpawnHealth(Vector2 pos);
    void SpawnPowerUp(Vector2 pos, PowerUpType type);
    void SpawnWeaponLoot(Vector2 pos, WeaponType type);
    void SpawnAmmo(Vector2 pos);
    void SpawnArmor(Vector2 pos, float reduction);

    // Single entry point for "an enemy just died here" — rolls whether
    // anything drops at all (scaled by dropChanceMult), and if so, which
    // kind (health / power-up / ammo / armor).
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
