#pragma once

#include "Common.hpp"
#include "Entity.hpp"
#include "Projectile.hpp"
#include <memory>
#include <vector>

// Base class for all enemy AI variants. Owns position/health via Entity and
// adds contact-damage bookkeeping shared by every minion type.
class Enemy : public Entity {
public:
    Enemy(EnemyType type, Vector2 pos, float radius, float health, float speed, float damage);
    ~Enemy() override = default;

    // Runs AI behaviour for this enemy. `all` is the full roster (for
    // flocking); `projectiles` lets ranged minions/bosses spawn bullets.
    virtual void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                           ProjectileManager& projectiles) = 0;
    void Update(float dt, Rectangle bounds) override; // integrates velocity -> position, clamps to room, ticks cooldowns/status
    void Draw() const override;
    virtual void DrawDebug() const;

    EnemyType Type() const { return type_; }
    virtual bool IsBoss() const { return false; }

    // Multiplies health/contactDamage in place (used for both elite variants
    // and difficulty scaling; safe to call more than once, effects stack).
    void ScaleStats(float mult) {
        health *= mult;
        maxHealth = health;
        contactDamage *= mult;
    }

    // Contact damage: returns the damage to apply to the player if the
    // per-enemy cooldown has elapsed, and resets the cooldown. Returns 0 if
    // still on cooldown or not currently touching.
    float TryContactDamage();

    // Elite variant: scales up this enemy's stats and flags it for tinted
    // rendering / bigger death FX. Must be called right after construction,
    // before the enemy has taken any damage.
    void MakeElite();
    bool IsElite() const { return elite_; }

    // Weapon-synergy status: sword hits mark an enemy for a bonus-damage
    // window that the blaster checks (see cfg::kMarkedDuration).
    void MarkForBonus(float duration) { markedTimer_ = duration; }
    bool IsMarked() const { return markedTimer_ > 0.0f; }

    // Boss attacks that need systems UpdateAI doesn't have direct access to
    // (radial player damage, minion summons) queue a request here instead;
    // LevelManager polls every enemy generically after the AI tick so this
    // doesn't need a Boss-specific downcast. No-ops for regular minions.
    struct AoeRequest { Vector2 origin; float radius; float damage; float impulseStrength; };
    struct SummonRequest { EnemyType type; Vector2 pos; };
    virtual bool ConsumeAoeRequest(AoeRequest&) { return false; }
    virtual bool ConsumeSummonRequest(SummonRequest&) { return false; }

    float speed;
    float contactDamage;

protected:
    Color EliteTint(Color base) const;
    void DrawEliteRing() const;

    EnemyType type_;
    float contactCooldownTimer_ = 0.0f;
    bool elite_ = false;
    float markedTimer_ = 0.0f;
};

// Fast swarm minion. Chases the player directly, with boids-style separation
// so a horde doesn't collapse into a single point.
class GlazedChaser : public Enemy {
public:
    explicit GlazedChaser(Vector2 pos);
    void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                  ProjectileManager& projectiles) override;
    void Draw() const override;
};

// Heavy minion: stalks the player at range, telegraphs, then charges in a
// straight line at high speed.
class TwistCharger : public Enemy {
public:
    explicit TwistCharger(Vector2 pos);
    void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                  ProjectileManager& projectiles) override;
    void Draw() const override;

private:
    enum class State { Stalk, Telegraph, Charging, Recover };
    State state_ = State::Stalk;
    float stateTimer_ = 0.0f;
    Vector2 chargeDir_{1, 0};
};

// Ranged minion: keeps its distance and lobs slow shots at the player.
class CheddarShooter : public Enemy {
public:
    explicit CheddarShooter(Vector2 pos);
    void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                  ProjectileManager& projectiles) override;
    void Draw() const override;

private:
    float fireTimer_ = cfg::kCheddarShooterFireInterval * 0.5f; // stagger first shot
};

// Factory helper used by LevelManager. `elite` upgrades stats and marks the
// enemy for tinted rendering / bigger death FX; `statMult` additionally
// scales health/damage for difficulty tuning (applied after elite scaling).
std::unique_ptr<Enemy> MakeEnemy(EnemyType type, Vector2 pos, bool elite = false, float statMult = 1.0f);
