#include "LevelManager.hpp"
#include <utility>

void LevelManager::LoadLevel(int index) {
    levelIndex_ = index;
    level_ = MakeLevel(index);
    activeRoomIndex_ = -1;
    enemies_.clear();
    activeBoss_ = nullptr;
    aliveCount_ = 0;
    interWaveTimer_ = 0.0f;
    levelJustCompleted_ = false;
    ActivateRoom(0);
    activeRoomIndex_ = 0;
    aliveCount_ = static_cast<int>(enemies_.size());
}

void LevelManager::StartGame() { LoadLevel(0); }

bool LevelManager::HasNextLevel() const { return levelIndex_ + 1 < kLevelCount; }

void LevelManager::AdvanceToNextLevel() { LoadLevel(levelIndex_ + 1); }

int LevelManager::ComputeRoomIndexForX(float x) const {
    for (size_t i = 0; i < level_.rooms.size(); ++i) {
        const Room& room = level_.rooms[i];
        if (x < room.bounds.x + room.bounds.width || i + 1 == level_.rooms.size()) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void LevelManager::ActivateRoom(int index) {
    Room& room = level_.rooms[index];
    if (room.entered) return;
    room.entered = true;

    if (room.isBossRoom) {
        Vector2 center{room.bounds.x + room.bounds.width * 0.5f, room.bounds.y + room.bounds.height * 0.5f};
        auto boss = MakeBoss(room.bossType, center, tuning_.enemyStatMult);
        activeBoss_ = static_cast<Boss*>(boss.get());
        bossWasPhase2_ = false;
        enemies_.push_back(std::move(boss));
    } else {
        SpawnNextSubWave(room);
    }
}

void LevelManager::SpawnNextSubWave(Room& room) {
    if (room.nextWaveIndex >= room.waves.size()) {
        room.cleared = true;
        return;
    }
    const auto& batch = room.waves[room.nextWaveIndex];
    Rectangle playArea = room.PlayArea();
    for (const SpawnBatch& b : batch) {
        int count = std::max(1, static_cast<int>(std::round(static_cast<float>(b.count) * tuning_.enemyCountMult)));
        // Harder difficulties make elites show up more often by shrinking
        // the "every Nth" gap (never below every-1, i.e. all elite).
        int eliteEvery = b.eliteEvery > 0
                             ? std::max(1, static_cast<int>(std::round(static_cast<float>(b.eliteEvery) / tuning_.eliteChanceMult)))
                             : 0;
        for (int i = 0; i < count; ++i) {
            bool elite = eliteEvery > 0 && ((i + 1) % eliteEvery == 0);
            Vector2 pos = mathutil::RandomEdgePosition(20.0f, playArea);
            enemies_.push_back(MakeEnemy(b.type, pos, elite, tuning_.enemyStatMult));
        }
    }
    room.nextWaveIndex++;
}

Rectangle LevelManager::CurrentRoomPlayArea() const {
    const Room& room = level_.rooms[activeRoomIndex_];
    float left = room.bounds.x + cfg::kRoomWallMargin;
    float right = room.bounds.x + room.bounds.width - cfg::kRoomWallMargin;

    // Previous room is always already cleared by construction (you can't be
    // standing in room N without having cleared room N-1), so backward
    // travel is never blocked.
    if (activeRoomIndex_ > 0) {
        left = level_.rooms[activeRoomIndex_ - 1].bounds.x + cfg::kRoomWallMargin;
    }
    // Forward travel opens once this room is cleared.
    if (room.cleared && activeRoomIndex_ + 1 < static_cast<int>(level_.rooms.size())) {
        const Room& next = level_.rooms[activeRoomIndex_ + 1];
        right = next.bounds.x + next.bounds.width - cfg::kRoomWallMargin;
    }

    return Rectangle{left, room.bounds.y + cfg::kRoomWallMargin, right - left,
                      room.bounds.height - 2.0f * cfg::kRoomWallMargin};
}

Rectangle LevelManager::CurrentRoomCameraBounds() const { return level_.rooms[activeRoomIndex_].bounds; }

Vector2 LevelManager::LevelStartSpawn() const {
    const Room& room0 = level_.rooms[0];
    return Vector2{room0.bounds.x + cfg::kRoomWallMargin + 60.0f, room0.bounds.y + room0.bounds.height * 0.5f};
}

void LevelManager::Update(float dt, Player& player, ParticleSystem& particles, ProjectileManager& projectiles,
                           ScreenShake& shake, PickupManager& pickups, AudioManager& audio, bool playerBlocking) {
    levelJustCompleted_ = false;

    int newIdx = ComputeRoomIndexForX(player.position.x);
    if (newIdx != activeRoomIndex_) activeRoomIndex_ = newIdx;
    Room& room = level_.rooms[activeRoomIndex_];
    if (!room.entered) ActivateRoom(activeRoomIndex_);

    Rectangle playArea = room.PlayArea();

    std::vector<std::pair<EnemyType, Vector2>> pendingSummons;

    for (auto& e : enemies_) {
        if (!e->IsAlive()) continue;
        e->UpdateAI(dt, player.position, enemies_, projectiles);
        e->Update(dt, playArea);

        float dist = Vector2Distance(e->position, player.position);
        if (dist < e->radius + player.radius) {
            float dmg = e->TryContactDamage();
            if (dmg > 0.0f && playerBlocking) dmg *= (1.0f - cfg::kShieldBlockMitigation);
            if (dmg > 0.0f && player.ApplyContactDamage(dmg)) {
                Vector2 away = Vector2Subtract(player.position, e->position);
                if (Vector2LengthSqr(away) > 0.0001f) {
                    player.velocity = Vector2Add(player.velocity, Vector2Scale(Vector2Normalize(away), 180.0f));
                }
                particles.SpawnBurst(player.position, 8, RED, 60.0f, 200.0f, 0.15f, 0.35f);
                audio.Play(Sfx::PlayerHurt, 0.8f, 0.15f);
            }
        }

        Enemy::AoeRequest aoe;
        if (e->ConsumeAoeRequest(aoe)) {
            particles.SpawnBurst(aoe.origin, 40, Color{255, 140, 60, 255}, 120.0f, 480.0f, 0.3f, 0.7f, 3.0f, 7.0f);
            shake.Trigger(0.4f, 14.0f);
            pendingHitStop_ = std::max(pendingHitStop_, cfg::kHitStopHeavy);
            audio.Play(Sfx::BossSlam);
            float pdist = Vector2Distance(player.position, aoe.origin);
            if (pdist < aoe.radius) {
                float falloff = 1.0f - mathutil::Clamp01(pdist / aoe.radius);
                if (player.ApplyContactDamage(aoe.damage * falloff)) audio.Play(Sfx::PlayerHurt, 0.9f, 0.1f);
                player.velocity = Vector2Add(player.velocity, mathutil::RadialImpulse(player.position, aoe.origin, aoe.impulseStrength, dt));
            }
        }

        Enemy::SummonRequest summon;
        if (e->ConsumeSummonRequest(summon)) {
            pendingSummons.emplace_back(summon.type, summon.pos);
        }
    }

    for (auto& [type, pos] : pendingSummons) {
        Vector2 clamped = mathutil::ClampToRoom(pos, cfg::kGlazedChaserRadius, playArea);
        enemies_.push_back(MakeEnemy(type, clamped));
    }

    if (activeBoss_ && activeBoss_->InPhase2() && !bossWasPhase2_) {
        bossWasPhase2_ = true;
        pendingHitStop_ = std::max(pendingHitStop_, cfg::kHitStopHeavy);
        shake.Trigger(0.3f, 10.0f);
        audio.Play(Sfx::BossPhase2);
    }

    // Hazard zones: enemies take steady damage; the player is gated by its
    // own contact-iframe timer so a dash still lets them dodge through.
    for (const Hazard& hz : room.hazards) {
        Vector2 center{room.bounds.x + hz.center.x, room.bounds.y + hz.center.y};
        if (Vector2Distance(player.position, center) < hz.radius + player.radius) {
            if (player.ApplyContactDamage(cfg::kHazardDamagePerSec * cfg::kContactIFrames)) {
                audio.Play(Sfx::PlayerHurt, 0.6f, 0.15f);
            }
        }
        for (auto& e : enemies_) {
            if (!e->IsAlive()) continue;
            if (Vector2Distance(e->position, center) < hz.radius + e->radius) {
                e->TakeDamage(cfg::kHazardDamagePerSec * dt);
            }
        }
    }

    // Cull dead enemies (drop pickups + clear the boss pointer as needed).
    bool bossWasActive = activeBoss_ != nullptr;
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        if (!(*it)->IsAlive()) {
            if (activeBoss_ == it->get()) activeBoss_ = nullptr;
            pickups.RollAndSpawnDrop((*it)->position);
            it = enemies_.erase(it);
        } else {
            ++it;
        }
    }
    aliveCount_ = static_cast<int>(enemies_.size());

    if (aliveCount_ == 0) {
        if (room.isBossRoom) {
            if (!room.cleared) {
                room.cleared = true;
                if (bossWasActive) levelJustCompleted_ = true;
            }
        } else {
            interWaveTimer_ -= dt;
            if (interWaveTimer_ <= 0.0f) {
                bool wasCleared = room.cleared;
                SpawnNextSubWave(room);
                if (!wasCleared && room.cleared) audio.Play(Sfx::GateUnlock);
                interWaveTimer_ = kInterWaveDelay;
                aliveCount_ = static_cast<int>(enemies_.size());
            }
        }
    } else {
        interWaveTimer_ = kInterWaveDelay;
    }
}

void LevelManager::KillAllEnemies(ParticleSystem& particles, ComboTracker& combo, int& score) {
    for (auto& e : enemies_) {
        if (!e->IsAlive() || e->IsBoss()) continue;
        e->TakeDamage(e->health + 1.0f);
        particles.SpawnBurst(e->position, 14, Color{200, 40, 40, 255}, 60.0f, 260.0f, 0.25f, 0.5f);
        score += combo.RegisterKill(20);
    }
}

float LevelManager::ConsumeHitStopRequest() {
    float v = pendingHitStop_;
    pendingHitStop_ = 0.0f;
    return v;
}

float LevelManager::BossHealthFrac() const {
    if (!activeBoss_) return 0.0f;
    return activeBoss_->maxHealth > 0.0f ? activeBoss_->health / activeBoss_->maxHealth : 0.0f;
}

const std::string& LevelManager::BossName() const {
    static const std::string kEmpty;
    return activeBoss_ ? activeBoss_->BossName() : kEmpty;
}

void LevelManager::Draw(bool debug) const {
    for (const auto& e : enemies_) {
        if (!e->IsAlive()) continue;
        e->Draw();
        if (debug) e->DrawDebug();
    }
}
