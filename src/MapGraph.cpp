#include "MapGraph.hpp"

namespace {

Zone MakeZone(std::string name, Rectangle bounds) {
    Zone zone;
    zone.name = std::move(name);
    zone.bounds = bounds;
    float m = cfg::kRoomWallMargin + 20.0f;
    // Five fixed boarded-up windows around the zone's perimeter; zombies
    // breach through one at random each spawn (ZombiesDirector::SpawnOne).
    zone.spawnPoints = {
        SpawnWindow{Vector2{bounds.x + m, bounds.y + m}},
        SpawnWindow{Vector2{bounds.x + bounds.width - m, bounds.y + m}},
        SpawnWindow{Vector2{bounds.x + m, bounds.y + bounds.height - m}},
        SpawnWindow{Vector2{bounds.x + bounds.width - m, bounds.y + bounds.height - m}},
        SpawnWindow{Vector2{bounds.x + bounds.width * 0.5f, bounds.y + m}},
    };
    return zone;
}

}  // namespace

void MapGraph::BuildDefaultMap() {
    zones_.clear();
    barriers_.clear();

    // Each room gets its own proportions instead of a uniform box, so the
    // map reads as distinct spaces rather than the same rectangle four
    // times: a wide-open Cafeteria, a narrow Storage Hall corridor, a
    // squarish Industrial Kitchen, and a wide Deep Freezer finale room.
    // True branching topology (a zone reachable two ways) is out of scope —
    // MapGraph::ZoneIndexContaining is 1-D X-slab logic and would need a
    // real point-in-rect rework to support that safely; varying each zone's
    // width/height is the low-risk version of "the map feels dry."
    float x = 0.0f;
    zones_.push_back(MakeZone("Cafeteria", Rectangle{x, 0.0f, 2200.0f, 1200.0f}));
    x += 2200.0f;
    zones_.push_back(MakeZone("Storage Hall", Rectangle{x, 0.0f, 1500.0f, 750.0f}));
    x += 1500.0f;
    zones_.push_back(MakeZone("Industrial Kitchen", Rectangle{x, 0.0f, 1750.0f, 1350.0f}));
    x += 1750.0f;
    zones_.push_back(MakeZone("Deep Freezer", Rectangle{x, 0.0f, 2300.0f, 1150.0f}));

    zones_[0].active = true; // spawn room, active immediately

    // Themed set-dressing per room (room-local coordinates, same convention
    // as Room.hpp's Obstacle/Hazard); positions avoid the wall-buy/mystery
    // box/spawn spots placed further below.
    zones_[0].obstacles = {
        Obstacle{Vector2{580.0f, 800.0f}, 34.0f, ObstacleKind::Table},
        Obstacle{Vector2{1100.0f, 970.0f}, 34.0f, ObstacleKind::Table},
        Obstacle{Vector2{1680.0f, 800.0f}, 34.0f, ObstacleKind::Table},
    };
    zones_[1].obstacles = {
        Obstacle{Vector2{237.0f, 214.0f}, 40.0f, ObstacleKind::Shelf},
        Obstacle{Vector2{237.0f, 536.0f}, 40.0f, ObstacleKind::Shelf},
        Obstacle{Vector2{1263.0f, 214.0f}, 40.0f, ObstacleKind::Shelf},
        Obstacle{Vector2{1263.0f, 536.0f}, 40.0f, ObstacleKind::Shelf},
    };
    zones_[2].obstacles = {
        Obstacle{Vector2{461.0f, 1093.0f}, 34.0f, ObstacleKind::Counter},
        Obstacle{Vector2{875.0f, 386.0f}, 34.0f, ObstacleKind::Counter},
        Obstacle{Vector2{1336.0f, 1093.0f}, 34.0f, ObstacleKind::Counter},
    };
    zones_[3].obstacles = {
        Obstacle{Vector2{424.0f, 274.0f}, 32.0f, ObstacleKind::MeatRack},
        Obstacle{Vector2{1877.0f, 274.0f}, 32.0f, ObstacleKind::MeatRack},
        Obstacle{Vector2{1150.0f, 931.0f}, 32.0f, ObstacleKind::MeatRack},
    };

    auto makeBarrier = [this](int from, int to, int cost) {
        const Zone& a = zones_[static_cast<size_t>(from)];
        const Zone& b2 = zones_[static_cast<size_t>(to)];
        float gateX = a.bounds.x + a.bounds.width;
        float gateHeight = std::max(a.bounds.height, b2.bounds.height);
        Barrier b;
        b.bounds = Rectangle{gateX - 20.0f, 0.0f, 40.0f, gateHeight};
        b.cost = cost;
        b.fromZone = from;
        b.toZone = to;
        barriers_.push_back(b);
    };
    makeBarrier(0, 1, 750);  // Door 1
    makeBarrier(1, 2, 1000); // Door 2
    makeBarrier(2, 3, 1250); // Door 3
}

int MapGraph::ZoneIndexContaining(Vector2 pos) const {
    for (size_t i = 0; i < zones_.size(); ++i) {
        const Zone& z = zones_[i];
        if (pos.x < z.bounds.x + z.bounds.width || i + 1 == zones_.size()) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

Rectangle MapGraph::PlayAreaFor(int zoneIndex) const {
    const Zone& zone = zones_[static_cast<size_t>(zoneIndex)];
    float margin = cfg::kRoomWallMargin;
    float left = zone.bounds.x + margin;
    float right = zone.bounds.x + zone.bounds.width - margin;

    if (zoneIndex > 0) {
        left = zones_[static_cast<size_t>(zoneIndex - 1)].bounds.x + margin;
    }
    if (zoneIndex < static_cast<int>(barriers_.size()) && barriers_[static_cast<size_t>(zoneIndex)].cleared) {
        const Zone& next = zones_[static_cast<size_t>(zoneIndex + 1)];
        right = next.bounds.x + next.bounds.width - margin;
    }

    return Rectangle{left, zone.bounds.y + margin, right - left, zone.bounds.height - 2.0f * margin};
}

Barrier* MapGraph::NearbyUnclearedBarrier(Vector2 pos, float range) {
    return const_cast<Barrier*>(static_cast<const MapGraph&>(*this).NearbyUnclearedBarrier(pos, range));
}

const Barrier* MapGraph::NearbyUnclearedBarrier(Vector2 pos, float range) const {
    for (const Barrier& b : barriers_) {
        if (b.cleared) continue;
        Vector2 center{b.bounds.x + b.bounds.width * 0.5f, b.bounds.y + b.bounds.height * 0.5f};
        // Distance to the barrier's vertical line (clamped to its height) —
        // simple enough given barriers are always full-height doorways.
        float dy = Clamp(pos.y, b.bounds.y, b.bounds.y + b.bounds.height) - pos.y;
        float dx = center.x - pos.x;
        if (std::sqrt(dx * dx + dy * dy) <= range) return &b;
    }
    return nullptr;
}

bool MapGraph::TryClearBarrier(Barrier& barrier, Economy& economy) {
    if (barrier.cleared) return false;
    if (!economy.Spend(barrier.cost)) return false;
    barrier.cleared = true;
    zones_[static_cast<size_t>(barrier.toZone)].active = true;
    return true;
}

Vector2 MapGraph::StartSpawnPoint() const {
    const Zone& zone = zones_[0];
    return Vector2{zone.bounds.x + cfg::kRoomWallMargin + 60.0f, zone.bounds.y + zone.bounds.height * 0.5f};
}
