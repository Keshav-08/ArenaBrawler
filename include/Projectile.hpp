#pragma once

#include "Common.hpp"
#include <array>
#include <functional>
#include <vector>

enum class ProjectileKind { Bullet, Bomb };

struct Projectile {
    ProjectileKind kind = ProjectileKind::Bullet;
    Vector2 position{};
    Vector2 velocity{};
    float radius = 4.0f;
    float damage = 0.0f;
    float life = 0.0f;      // bullets: remaining lifetime: bombs: unused
    float fuse = 0.0f;      // bombs only: time until detonation
    float blastRadius = 0.0f; // bombs only
    bool hostile = false;   // bullets only: true for enemy/boss shots, resolved against the player instead of enemies
    float slowDuration = 0.0f; // bullets only: Zombies Mode's Freeze weapon; 0 = no slow effect
    bool active = false;
};

struct Explosion {
    Vector2 position{};
    float radius = 0.0f;
    float damage = 0.0f;
};

// Fixed-capacity object pool for both hitscan-speed bullets and lobbed bombs.
// Movement/lifetime/fuse bookkeeping lives here; actual gameplay effects
// (damage, impulses) are resolved by Game against the WaveManager roster so
// this class stays decoupled from Enemy.
class ProjectileManager {
public:
    void SpawnBullet(Vector2 pos, Vector2 vel, float damage, float radius, float life, bool hostile = false, float slowDuration = 0.0f);
    void SpawnBomb(Vector2 pos, Vector2 vel, float damage, float blastRadius, float fuse);

    // Advances all active projectiles, culls out-of-bounds/expired bullets,
    // and detonates bombs whose fuse has elapsed (pushing an Explosion into
    // the pending list, retrievable via PopExplosions). `bounds` is the
    // current room's play area.
    void Update(float dt, Rectangle bounds);
    void Draw() const;

    std::vector<Explosion> PopExplosions();

    std::array<Projectile, cfg::kMaxProjectiles>& Pool() { return pool_; }
    const std::array<Projectile, cfg::kMaxProjectiles>& Pool() const { return pool_; }

private:
    Projectile* AcquireSlot();

    std::array<Projectile, cfg::kMaxProjectiles> pool_{};
    std::vector<Explosion> pendingExplosions_;
};
