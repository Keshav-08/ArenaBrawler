#pragma once

#include "Audio.hpp"
#include "Boss.hpp"
#include "Common.hpp"
#include "Enemy.hpp"
#include "Level.hpp"
#include "ParticleSystem.hpp"
#include "Pickup.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include <memory>
#include <string>
#include <vector>

// Top-level progression driver: replaces the old infinite-wave WaveManager.
// Owns the current Level, tracks which room the player is standing in
// (rooms are laid out sequentially along world X — see Level.cpp), spawns a
// room's minion gauntlet on first entry, opens the gate to the next room
// once the roster is cleared, and spawns/tracks the level's boss in the
// final room.
class LevelManager {
public:
    void StartGame(PickupManager& pickups);         // loads level 0, room 0
    bool HasNextLevel() const;
    void AdvanceToNextLevel(PickupManager& pickups); // loads levelIndex_ + 1 at its room 0

    // Picked once on the level-1 intro banner; scales enemy counts/stats/
    // elite frequency for every room spawned from here on.
    void SetDifficulty(Difficulty d) { tuning_ = GetDifficultyTuning(d); }

    // Nuke power-up: instantly kills every non-boss enemy alive in the
    // current room (bosses are immune so the set-piece fight stays intact).
    void KillAllEnemies(ParticleSystem& particles, ComboTracker& combo, int& score);

    // Defers spawning an enemy until the next Update() call (safe to call
    // from a kill-resolution site that's mid-iteration over GetEnemies(),
    // e.g. PickleSplitter spawning its children on death).
    void QueueSpawn(EnemyType type, Vector2 pos, bool isChild = false) {
        pendingExternalSpawns_.push_back({type, pos, isChild});
    }

    void Update(float dt, Player& player, ParticleSystem& particles, ProjectileManager& projectiles,
                ScreenShake& shake, PickupManager& pickups, AudioManager& audio, bool playerBlocking);
    void Draw(bool debug) const;

    // One-shot: non-zero for the frame a heavy event (boss slam, boss
    // entering phase 2) fired. Game folds it into its hit-stop timer.
    float ConsumeHitStopRequest();

    // Play-area rect (inset by wall margin) that Player/Enemy/Projectile
    // clamp movement to; already open across any cleared boundary.
    Rectangle CurrentRoomPlayArea() const;
    // Full room rect (not inset) the camera frames.
    Rectangle CurrentRoomCameraBounds() const;
    // Spawn point for the start of the current level.
    Vector2 LevelStartSpawn() const;
    // Read-only access to the room the player is currently in, for drawing
    // its background/hazards/gate.
    const Room& CurrentRoom() const { return level_.rooms[activeRoomIndex_]; }
    bool HasNextRoom() const { return activeRoomIndex_ + 1 < static_cast<int>(level_.rooms.size()); }

    int LevelNumber() const { return levelIndex_ + 1; }
    int LevelCount() const { return kLevelCount; }
    bool IsFinalLevel() const { return levelIndex_ == kLevelCount - 1; }
    int RoomNumber() const { return activeRoomIndex_ + 1; }
    int RoomCount() const { return static_cast<int>(level_.rooms.size()); }
    const std::string& LevelName() const { return level_.name; }
    Biome CurrentBiome() const { return level_.biome; }
    int AliveCount() const { return aliveCount_; }

    bool HasActiveBoss() const { return activeBoss_ != nullptr; }
    float BossHealthFrac() const;
    const std::string& BossName() const;

    // Shrinking safe zone (boss rooms only): non-zero radius once the boss
    // room's fight has started. Game reads this to render the boundary and
    // to know when to warn the player.
    bool HasActiveZone() const;
    float CurrentZoneRadius() const;
    Vector2 CurrentZoneCenter() const;

    // One-shot: true for the frame the level's boss died. Game reacts by
    // switching to the LevelComplete banner state.
    bool LevelJustCompleted() const { return levelJustCompleted_; }

    std::vector<std::unique_ptr<Enemy>>& GetEnemies() { return enemies_; }

private:
    void LoadLevel(int index, PickupManager& pickups);
    void ActivateRoom(int index, PickupManager& pickups);
    void SpawnNextSubWave(Room& room);
    int ComputeRoomIndexForX(float x) const;

    Level level_;
    int levelIndex_ = 0;
    int activeRoomIndex_ = -1; // -1 forces ActivateRoom(0) on the first Update

    std::vector<std::unique_ptr<Enemy>> enemies_;
    Boss* activeBoss_ = nullptr; // non-owning, points into enemies_ while the boss room's boss is alive
    int aliveCount_ = 0;
    float interWaveTimer_ = 0.0f;
    bool levelJustCompleted_ = false;
    bool bossWasPhase2_ = false;
    float pendingHitStop_ = 0.0f;
    DifficultyTuning tuning_;

    struct PendingSpawn { EnemyType type; Vector2 pos; bool isChild; };
    std::vector<PendingSpawn> pendingExternalSpawns_;

    static constexpr float kInterWaveDelay = 1.6f;
};
