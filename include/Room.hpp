#pragma once

#include "Common.hpp"
#include <vector>

// One spawn batch within a room's gauntlet: `count` enemies of `type`,
// with every `eliteEvery`-th one (if non-zero) upgraded to an elite variant.
struct SpawnBatch {
    EnemyType type;
    int count;
    int eliteEvery = 0; // 0 = no elites in this batch
};

// A static damage-over-time zone inside a room (fire pit, gas cloud, ...).
// `center` is room-local (relative to the room's top-left corner, i.e. in
// [0, kRoomWidth] x [0, kRoomHeight]) since Level data is authored before a
// room's final world-space offset is known; callers add room.bounds.x/y.
struct Hazard {
    Vector2 center{};
    float radius = 60.0f;
    Color color = Color{230, 90, 40, 140};
};

// A single arena the player fights through. Rooms are laid out sequentially
// along the world X axis by Level; the boundary between room N and N+1 is
// sealed (an energy gate is drawn and the player is clamped on that edge)
// until room N is cleared.
struct Room {
    Rectangle bounds{};

    // Minion gauntlet: each inner vector is one sub-wave, spawned once the
    // previous sub-wave is fully cleared (same escalation feel as the old
    // WaveManager). Empty for boss rooms.
    std::vector<std::vector<SpawnBatch>> waves;

    bool isBossRoom = false;
    EnemyType bossType = EnemyType::BossBruiser; // only meaningful if isBossRoom

    std::vector<Hazard> hazards;

    // --- Runtime state, mutated by LevelManager as the player progresses ---
    bool entered = false;   // camera/spawns activated once player first enters
    bool cleared = false;   // gate to the next room is open
    size_t nextWaveIndex = 0;

    // Inset room bounds that entities are clamped to (walls eat kRoomWallMargin).
    Rectangle PlayArea() const {
        return Rectangle{
            bounds.x + cfg::kRoomWallMargin,
            bounds.y + cfg::kRoomWallMargin,
            bounds.width - 2.0f * cfg::kRoomWallMargin,
            bounds.height - 2.0f * cfg::kRoomWallMargin,
        };
    }
};
