#pragma once

#include "Common.hpp"
#include "Enemy.hpp"
#include <string>

// Shared base for unique level-ending fights. Adds a named identity (for the
// boss HP bar HUD) and a phase flag that flips at half health; the AoE/summon
// request plumbing itself lives on Enemy (see RequestAoe/RequestSummon
// there) since regular minions ended up needing it too.
class Boss : public Enemy {
public:
    Boss(EnemyType type, std::string name, Vector2 pos, float radius, float health, float speed, float damage);

    const std::string& BossName() const { return name_; }
    bool InPhase2() const { return health <= maxHealth * cfg::kBossPhase2HealthFrac; }
    bool IsBoss() const override { return true; }

private:
    std::string name_;
};

// Melee bruiser: stalks, telegraphs, charges, then slams down a radial AoE
// on impact/recovery. Phase 2 (below half health) charges faster and slams
// harder.
class BossBruiser : public Boss {
public:
    explicit BossBruiser(Vector2 pos);
    void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                  ProjectileManager& projectiles) override;
    void Draw() const override;

private:
    enum class State { Stalk, Telegraph, Charging, Slam, Recover };
    State state_ = State::Stalk;
    float stateTimer_ = 0.0f;
    Vector2 chargeDir_{1, 0};
};

// Ranged summoner: hovers at range firing spread volleys; phase 2 adds
// periodic minion summons on top of faster volleys.
class BossCaster : public Boss {
public:
    explicit BossCaster(Vector2 pos);
    void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                  ProjectileManager& projectiles) override;
    void Draw() const override;

private:
    float volleyTimer_ = cfg::kCasterVolleyInterval * 0.5f;
    float summonTimer_ = cfg::kCasterSummonInterval;
};

// Desert boss: alternates stalking with a burrow-dash-emerge attack — dives
// underground (harmless, faint) and travels fast toward a point near the
// player, then erupts with a radial AoE and a brief vulnerable exposure.
// Phase 2 travels faster and hits harder on emerge.
class BossBurrower : public Boss {
public:
    explicit BossBurrower(Vector2 pos);
    void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                  ProjectileManager& projectiles) override;
    void Draw() const override;

private:
    enum class State { Stalk, Burrow, Travel, EmergeTelegraph, Exposed, Recover };
    State state_ = State::Stalk;
    float stateTimer_ = 0.0f;
    Vector2 travelTarget_{};
};

// Ice boss (final): periodic self-centered "ice nova" pulses that damage,
// knock back, and slow the player; phase 2 adds a full-circle shard burst on
// top of more frequent novas.
class BossFrost : public Boss {
public:
    explicit BossFrost(Vector2 pos);
    void UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                  ProjectileManager& projectiles) override;
    void Draw() const override;

private:
    float novaTimer_ = cfg::kFrostNovaInterval * 0.5f;
    float novaTelegraphTimer_ = 0.0f;
    bool novaCharging_ = false;
    float shardTimer_ = cfg::kFrostShardInterval;
};

// Factory used by LevelManager when a boss room is entered. `statMult`
// scales health/damage for difficulty tuning.
std::unique_ptr<Enemy> MakeBoss(EnemyType type, Vector2 pos, float statMult = 1.0f);
