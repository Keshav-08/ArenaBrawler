#include "ZombiesDirector.hpp"
#include "Boss.hpp"
#include <algorithm>

void ZombiesDirector::StartRound() {
    round_ = 1;
    zombiesToSpawn_ = ZombieCountForRound(round_);
    spawnTimer_ = 0.0f;
    downtime_ = false;
    downtimeTimer_ = 0.0f;
}

float ZombiesDirector::HealthForRound(int round) {
    int cappedRound = std::min(round, 9);
    float health = 100.0f + static_cast<float>(cappedRound - 1) * 50.0f;
    for (int r = 10; r <= round; ++r) health *= 1.1f;
    return health;
}

int ZombiesDirector::ZombieCountForRound(int round) {
    // Simple capped growth curve (spec leaves the exact formula open,
    // unlike the health formula it specifies explicitly).
    return std::min(6 + (round - 1) * 2, 30);
}

// Weighted pool: GlazedChaser stays the common baseline throughout (weight
// 3) while each special becomes eligible starting at its gate round and is
// mixed in at a modest rate rather than dominating the wave.
EnemyType ZombiesDirector::PickTypeForRound(int round) {
    std::vector<EnemyType> pool = {EnemyType::GlazedChaser, EnemyType::GlazedChaser, EnemyType::GlazedChaser};
    if (round >= kCheddarShooterRound) pool.push_back(EnemyType::CheddarShooter);
    if (round >= kTwistChargerRound) pool.push_back(EnemyType::TwistCharger);
    if (round >= kSodaBomberRound) pool.push_back(EnemyType::SodaBomber);
    if (round >= kPickleSplitterRound) pool.push_back(EnemyType::PickleSplitter);
    return pool[static_cast<size_t>(GetRandomValue(0, static_cast<int>(pool.size()) - 1))];
}

void ZombiesDirector::SpawnOne(const MapGraph& map, std::vector<std::unique_ptr<Enemy>>& zombies) {
    std::vector<const Zone*> activeZones;
    for (const Zone& z : map.Zones()) {
        if (z.active && !z.spawnPoints.empty()) activeZones.push_back(&z);
    }
    if (activeZones.empty()) return;

    const Zone* zone = activeZones[static_cast<size_t>(GetRandomValue(0, static_cast<int>(activeZones.size()) - 1))];
    Vector2 pos = zone->spawnPoints[static_cast<size_t>(GetRandomValue(0, static_cast<int>(zone->spawnPoints.size()) - 1))];

    // Rare heavy special: BossBurrower's dive/emerge pattern reused as a
    // mini-boss (health scaled down from full arena-boss tier), capped at
    // one alive at a time so it stays a spike, not a second boss fight.
    if (round_ >= kHeavySpecialRound) {
        bool heavyAlive = false;
        for (const auto& z : zombies) {
            if (z->IsAlive() && z->IsBoss()) {
                heavyAlive = true;
                break;
            }
        }
        if (!heavyAlive && GetRandomValue(0, 99) < kHeavySpecialChancePercent) {
            auto heavy = MakeBoss(EnemyType::BossBurrower, pos);
            if (heavy) {
                heavy->health = HealthForRound(round_) * kHeavySpecialHealthMult;
                heavy->maxHealth = heavy->health;
                zombies.push_back(std::move(heavy));
                return;
            }
        }
    }

    EnemyType type = PickTypeForRound(round_);
    auto zombie = MakeEnemy(type, pos);
    if (zombie) {
        zombie->health = HealthForRound(round_);
        zombie->maxHealth = zombie->health;
        // GlazedChaser's base speed (210) was tuned for Story Mode's
        // squishy 22-HP swarm unit; reused as a much tankier zombie at
        // full speed it reads as an unfair rush. Slow it to a shamble.
        // Applied uniformly to every special too for a consistent pace.
        zombie->speed *= kZombieSpeedMult;
        zombies.push_back(std::move(zombie));
    }
}

void ZombiesDirector::Update(float dt, const MapGraph& map, std::vector<std::unique_ptr<Enemy>>& zombies) {
    int aliveCount = 0;
    for (const auto& z : zombies) {
        if (z->IsAlive()) aliveCount++;
    }

    if (downtime_) {
        downtimeTimer_ -= dt;
        if (downtimeTimer_ <= 0.0f) {
            downtime_ = false;
            round_++;
            zombiesToSpawn_ = ZombieCountForRound(round_);
            spawnTimer_ = 0.0f;
        }
        return;
    }

    if (zombiesToSpawn_ <= 0 && aliveCount == 0) {
        downtime_ = true;
        downtimeTimer_ = kDowntimeDuration;
        return;
    }

    if (zombiesToSpawn_ > 0 && aliveCount < kMaxConcurrentAlive) {
        spawnTimer_ -= dt;
        if (spawnTimer_ <= 0.0f) {
            SpawnOne(map, zombies);
            zombiesToSpawn_--;
            spawnTimer_ = kSpawnInterval;
        }
    }
}
