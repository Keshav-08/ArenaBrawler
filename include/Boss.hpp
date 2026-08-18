#pragma once

#include "Common.hpp"
#include "Enemy.hpp"
#include <string>

// Shared base for unique level-ending fights. Adds a named identity (for the
// boss HP bar HUD), a phase flag that flips at half health, and two
// "pending request" outboxes so attacks that need systems UpdateAI doesn't
// have direct access to (radial player damage, minion summons) can be
// resolved by LevelManager after the AI tick, without widening Enemy's
// UpdateAI signature just for bosses.
class Boss : public Enemy {
public:
    Boss(EnemyType type, std::string name, Vector2 pos, float radius, float health, float speed, float damage);

    const std::string& BossName() const { return name_; }
    bool InPhase2() const { return health <= maxHealth * cfg::kBossPhase2HealthFrac; }
    bool IsBoss() const override { return true; }

    // Non-null only for the one frame the attack fired; consuming clears it.
    bool ConsumeAoeRequest(AoeRequest& out) override;
    bool ConsumeSummonRequest(SummonRequest& out) override;

protected:
    void RequestAoe(Vector2 origin, float radius, float damage, float impulseStrength);
    void RequestSummon(EnemyType type, Vector2 pos);

private:
    std::string name_;
    bool hasAoeRequest_ = false;
    AoeRequest aoeRequest_{};
    bool hasSummonRequest_ = false;
    SummonRequest summonRequest_{};
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

// Factory used by LevelManager when a boss room is entered. `statMult`
// scales health/damage for difficulty tuning.
std::unique_ptr<Enemy> MakeBoss(EnemyType type, Vector2 pos, float statMult = 1.0f);
