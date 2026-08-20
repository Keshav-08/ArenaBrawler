#include "Projectile.hpp"

Projectile* ProjectileManager::AcquireSlot() {
    for (auto& p : pool_) {
        if (!p.active) return &p;
    }
    return nullptr; // pool exhausted; drop the spawn request
}

void ProjectileManager::SpawnBullet(Vector2 pos, Vector2 vel, float damage, float radius, float life, bool hostile, float slowDuration) {
    Projectile* p = AcquireSlot();
    if (!p) return;
    *p = Projectile{};
    p->kind = ProjectileKind::Bullet;
    p->position = pos;
    p->velocity = vel;
    p->damage = damage;
    p->radius = radius;
    p->life = life;
    p->hostile = hostile;
    p->slowDuration = slowDuration;
    p->active = true;
}

void ProjectileManager::SpawnBomb(Vector2 pos, Vector2 vel, float damage, float blastRadius, float fuse) {
    Projectile* p = AcquireSlot();
    if (!p) return;
    *p = Projectile{};
    p->kind = ProjectileKind::Bomb;
    p->position = pos;
    p->velocity = vel;
    p->damage = damage;
    p->radius = cfg::kBombRadius;
    p->blastRadius = blastRadius;
    p->fuse = fuse;
    p->active = true;
}

void ProjectileManager::Update(float dt, Rectangle bounds) {
    for (auto& p : pool_) {
        if (!p.active) continue;

        p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));

        if (p.kind == ProjectileKind::Bullet) {
            p.life -= dt;
            bool outOfBounds = p.position.x < bounds.x || p.position.x > bounds.x + bounds.width ||
                                p.position.y < bounds.y || p.position.y > bounds.y + bounds.height;
            if (p.life <= 0.0f || outOfBounds) {
                p.active = false;
            }
        } else { // Bomb
            // Lobbed bomb decelerates (rolling friction) as it skids to a stop.
            p.velocity = Vector2Scale(p.velocity, std::pow(0.90f, dt * 60.0f));
            p.position = mathutil::ClampToRoom(p.position, p.radius, bounds);
            p.fuse -= dt;
            if (p.fuse <= 0.0f) {
                pendingExplosions_.push_back(Explosion{p.position, p.blastRadius, p.damage});
                p.active = false;
            }
        }
    }
}

std::vector<Explosion> ProjectileManager::PopExplosions() {
    std::vector<Explosion> out = std::move(pendingExplosions_);
    pendingExplosions_.clear();
    return out;
}

void ProjectileManager::Draw() const {
    for (const auto& p : pool_) {
        if (!p.active) continue;
        if (p.kind == ProjectileKind::Bullet) {
            Color c = p.hostile ? Color{230, 80, 150, 255} : Color{255, 210, 90, 255};
            DrawCircleV(p.position, p.radius, c);
            if (p.hostile) DrawCircleLines(static_cast<int>(p.position.x), static_cast<int>(p.position.y), p.radius + 2.0f, Fade(RED, 0.6f));
        } else {
            // Pulses faster as the fuse runs down.
            float pulse = 0.5f + 0.5f * std::sin(GetTime() * (8.0 + (1.2 - p.fuse) * 10.0));
            float t = mathutil::Clamp01(1.0f - p.fuse / cfg::kBombFuse);
            Color from{60, 160, 60, 255};
            Color to = RED;
            Color c{
                static_cast<unsigned char>(from.r + (to.r - from.r) * t),
                static_cast<unsigned char>(from.g + (to.g - from.g) * t),
                static_cast<unsigned char>(from.b + (to.b - from.b) * t),
                255,
            };
            DrawCircleV(p.position, p.radius + pulse * 1.5f, c);
            DrawCircleLines(static_cast<int>(p.position.x), static_cast<int>(p.position.y), p.blastRadius, Fade(RED, 0.12f));
        }
    }
}
