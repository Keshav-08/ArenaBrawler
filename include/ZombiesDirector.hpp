#pragma once

#include "Enemy.hpp"
#include "MapGraph.hpp"
#include <memory>
#include <vector>

// Infinite round loop for Zombies Mode: spawns zombies one at a time from
// currently-active zones (see MapGraph), scales their health per round, and
// runs a 5s downtime once a round is fully spawned-and-cleared. Reuses
// Story Mode's existing AI types as round-gated "specials" instead of
// writing new Enemy subclasses — each is already generic (UpdateAI only
// needs a roster + ProjectileManager, no LevelManager) and already has
// distinct behavior. GlazedChaser is the common baseline throughout; its
// boids separation keeps hordes from collapsing into a single point.
class ZombiesDirector {
public:
    void StartRound(); // (re)starts at round 1 — call from ZombiesMode::Enter()
    void Update(float dt, const MapGraph& map, std::vector<std::unique_ptr<Enemy>>& zombies);

    int Round() const { return round_; }
    bool InDowntime() const { return downtime_; }
    float DowntimeRemaining() const { return downtimeTimer_; }

    // Health(R) = 100 + (R-1)*50 for R<=9; compounds *1.1 per round above 9.
    static float HealthForRound(int round);

private:
    static int ZombieCountForRound(int round);
    static EnemyType PickTypeForRound(int round);
    void SpawnOne(const MapGraph& map, std::vector<std::unique_ptr<Enemy>>& zombies);

    int round_ = 0;
    int zombiesToSpawn_ = 0;
    float spawnTimer_ = 0.0f;
    bool downtime_ = false;
    float downtimeTimer_ = 0.0f;

    static constexpr float kSpawnInterval = 1.2f;
    static constexpr int kMaxConcurrentAlive = 12;
    static constexpr float kDowntimeDuration = 5.0f;
    static constexpr float kZombieSpeedMult = 0.55f; // GlazedChaser's base speed was tuned for a squishy Story Mode swarmer

    // Round-gated special eligibility (see PickTypeForRound).
    static constexpr int kCheddarShooterRound = 4;
    static constexpr int kTwistChargerRound = 6;
    static constexpr int kSodaBomberRound = 8;
    static constexpr int kPickleSplitterRound = 10;

    // Heavy special: BossBurrower reused as a mini-boss, capped at one alive.
    static constexpr int kHeavySpecialRound = 12;
    static constexpr int kHeavySpecialChancePercent = 15;
    static constexpr float kHeavySpecialHealthMult = 3.0f;
};
