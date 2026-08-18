#include "Level.hpp"

namespace {

// Assigns sequential world-space bounds to each room (room 0 at x=0, each
// subsequent room immediately to the right of the previous one) and returns
// the assembled level.
Level BuildLevel(std::string name, std::vector<Room> rooms) {
    float x = 0.0f;
    for (Room& room : rooms) {
        room.bounds = Rectangle{x, 0.0f, cfg::kRoomWidth, cfg::kRoomHeight};
        x += cfg::kRoomWidth;
    }
    Level level;
    level.name = std::move(name);
    level.rooms = std::move(rooms);
    return level;
}

Level MakeLevel1() {
    Room room0;
    room0.waves = {
        {{EnemyType::GlazedChaser, 5}},
        {{EnemyType::GlazedChaser, 4}, {EnemyType::TwistCharger, 1}},
    };

    Room room1;
    room1.waves = {
        {{EnemyType::GlazedChaser, 6}, {EnemyType::CheddarShooter, 2}},
        {{EnemyType::GlazedChaser, 4, /*eliteEvery=*/3}, {EnemyType::TwistCharger, 2}},
    };

    Room boss;
    boss.isBossRoom = true;
    boss.bossType = EnemyType::BossBruiser;

    return BuildLevel("Wave 1: The Cold Cut Aisle", {room0, room1, boss});
}

Level MakeLevel2() {
    Room room0;
    room0.waves = {
        {{EnemyType::GlazedChaser, 7}, {EnemyType::CheddarShooter, 3}},
        {{EnemyType::TwistCharger, 2}, {EnemyType::CheddarShooter, 2, /*eliteEvery=*/2}},
    };
    room0.hazards.push_back(Hazard{Vector2{cfg::kRoomWidth * 0.5f, cfg::kRoomHeight * 0.5f}, 70.0f});

    Room room1;
    room1.waves = {
        {{EnemyType::GlazedChaser, 8, /*eliteEvery=*/3}, {EnemyType::TwistCharger, 2}},
        {{EnemyType::CheddarShooter, 4}, {EnemyType::TwistCharger, 2, /*eliteEvery=*/2}},
    };
    room1.hazards.push_back(Hazard{Vector2{cfg::kRoomWidth * 0.3f, cfg::kRoomHeight * 0.25f}, 60.0f});
    room1.hazards.push_back(Hazard{Vector2{cfg::kRoomWidth * 0.7f, cfg::kRoomHeight * 0.75f}, 60.0f});

    Room boss;
    boss.isBossRoom = true;
    boss.bossType = EnemyType::BossCaster;

    return BuildLevel("Wave 2: The Buffet Line", {room0, room1, boss});
}

}  // namespace

Level MakeLevel(int index) {
    switch (index) {
        case 0: return MakeLevel1();
        case 1: return MakeLevel2();
        default: return MakeLevel1();
    }
}
