#pragma once

#include "Common.hpp"

// Lightweight base class shared by Player and Enemy: a physical body with
// position/velocity, a circular collider, and hit points.
class Entity {
public:
    Entity(Vector2 pos, float radius, float health)
        : position(pos), velocity{0, 0}, radius(radius), health(health), maxHealth(health) {}

    virtual ~Entity() = default;

    // `bounds` is the current room's play area; implementations clamp
    // position to it (see mathutil::ClampToRoom).
    virtual void Update(float dt, Rectangle bounds) { (void)dt; (void)bounds; }
    virtual void Draw() const {}

    bool IsAlive() const { return alive; }

    // Returns true if this call killed the entity.
    bool TakeDamage(float dmg) {
        if (!alive) return false;
        health -= dmg;
        if (health <= 0.0f) {
            health = 0.0f;
            alive = false;
            return true;
        }
        return false;
    }

    Vector2 position{};
    Vector2 velocity{};
    float radius;
    float health;
    float maxHealth;
    bool alive = true;
};
