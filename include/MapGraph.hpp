#pragma once

#include "Common.hpp"
#include "Economy.hpp"
#include "Room.hpp"
#include <string>
#include <vector>

// A boarded-up spawn point: zombies breach through it instead of appearing
// out of nowhere. `breached` flips permanently true the first time
// ZombiesDirector::SpawnOne uses it (no re-boarding), so it reads as a
// broken-open window for the rest of the run.
struct SpawnWindow {
    Vector2 position{};
    bool breached = false;
};

// A connected zone in the Zombies Mode map. `spawnPoints` (world-space)
// only produce zombies once `active` — flipped true when the barrier
// leading into this zone is cleared (see MapGraph::TryClearBarrier).
// `obstacles` reuses Story Mode's Obstacle/ObstacleKind type (see
// Room.hpp) for themed set-dressing (tables, shelves, ...); `center` is
// room-local like everywhere else that type is used, so callers add
// bounds.x/y.
struct Zone {
    std::string name;
    Rectangle bounds{};
    std::vector<SpawnWindow> spawnPoints;
    std::vector<Obstacle> obstacles;
    bool active = false;
};

// An interactable "clear the debris" barrier between two zones. Blocks
// player movement from `fromZone` into `toZone` until cleared.
struct Barrier {
    Rectangle bounds{};
    int cost = 0;
    bool cleared = false;
    int fromZone = 0;
    int toZone = 0;

    std::string PromptText() const { return "[E] Clear Debris - Cost: " + std::to_string(cost); }
};

// The Zombies Mode floor plan: Cafeteria -> Storage Hall -> Industrial
// Kitchen -> Deep Freezer, laid out sequentially along world X (same zone
// size as Story Mode's rooms, reused for a consistent sense of scale).
// Genuinely a graph (Zone nodes + Barrier edges) even though the default
// map happens to be a single path — BuildDefaultMap could be extended to
// branch without changing this class.
class MapGraph {
public:
    void BuildDefaultMap();

    std::vector<Zone>& Zones() { return zones_; }
    const std::vector<Zone>& Zones() const { return zones_; }
    std::vector<Barrier>& Barriers() { return barriers_; }
    const std::vector<Barrier>& Barriers() const { return barriers_; }

    int ZoneIndexContaining(Vector2 pos) const;

    // Player/enemy clamp rect for the zone at `zoneIndex`: open backward
    // into any previous zone (always reachable once you've been there),
    // open forward only once the connecting barrier is cleared. Mirrors
    // LevelManager::CurrentRoomPlayArea's approach.
    Rectangle PlayAreaFor(int zoneIndex) const;
    Rectangle CameraBoundsFor(int zoneIndex) const { return zones_[static_cast<size_t>(zoneIndex)].bounds; }

    Barrier* NearbyUnclearedBarrier(Vector2 pos, float range);
    const Barrier* NearbyUnclearedBarrier(Vector2 pos, float range) const;
    // Spends `barrier.cost` from `economy`; on success flips the barrier
    // cleared and activates the zone it leads to.
    bool TryClearBarrier(Barrier& barrier, Economy& economy);

    Vector2 StartSpawnPoint() const;

private:
    std::vector<Zone> zones_;
    std::vector<Barrier> barriers_;
};
