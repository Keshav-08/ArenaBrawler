#include "Level.hpp"

namespace {

// Assigns sequential world-space bounds to each room (room 0 at x=0, each
// subsequent room immediately to the right of the previous one) and returns
// the assembled level.
Level BuildLevel(std::string name, Biome biome, std::vector<Room> rooms) {
    float x = 0.0f;
    for (Room& room : rooms) {
        room.bounds = Rectangle{x, 0.0f, cfg::kRoomWidth, cfg::kRoomHeight};
        x += cfg::kRoomWidth;
    }
    Level level;
    level.name = std::move(name);
    level.biome = biome;
    level.rooms = std::move(rooms);
    return level;
}

// --- Level 1: Grass -------------------------------------------------------
Level MakeLevel1() {
    Room room0;
    room0.waves = {
        {{EnemyType::GlazedChaser, 5}},
        {{EnemyType::GlazedChaser, 4}, {EnemyType::TwistCharger, 1}},
    };
    room0.obstacles = {
        Obstacle{Vector2{500.0f, 250.0f}, 40.0f, ObstacleKind::Tree},
        Obstacle{Vector2{950.0f, 780.0f}, 30.0f, ObstacleKind::Bush},
        Obstacle{Vector2{1450.0f, 400.0f}, 40.0f, ObstacleKind::Tree},
    };
    room0.lootSpawns = {LootSpawn{Vector2{700.0f, 600.0f}, WeaponType::ChurroBlaster}};

    Room room1;
    room1.waves = {
        {{EnemyType::GlazedChaser, 6}, {EnemyType::CheddarShooter, 2}},
        {{EnemyType::GlazedChaser, 4, /*eliteEvery=*/3}, {EnemyType::TwistCharger, 2}},
    };
    room1.obstacles = {
        Obstacle{Vector2{400.0f, 300.0f}, 30.0f, ObstacleKind::Bush},
        Obstacle{Vector2{950.0f, 700.0f}, 42.0f, ObstacleKind::Tree},
        Obstacle{Vector2{1500.0f, 280.0f}, 40.0f, ObstacleKind::Tree},
    };
    room1.lootSpawns = {LootSpawn{Vector2{700.0f, 500.0f}, WeaponType::SkewerSpear}};

    Room boss;
    boss.isBossRoom = true;
    boss.bossType = EnemyType::BossBruiser;

    return BuildLevel("Level 1: The Glazed Meadows", Biome::Grass, {room0, room1, boss});
}

// --- Level 2: Desert -------------------------------------------------------
Level MakeLevel2() {
    Room room0;
    room0.waves = {
        {{EnemyType::GlazedChaser, 6}, {EnemyType::SodaBomber, 2}},
        {{EnemyType::PickleSplitter, 3}, {EnemyType::CheddarShooter, 2}},
    };
    room0.obstacles = {
        Obstacle{Vector2{300.0f, 820.0f}, 36.0f, ObstacleKind::Cactus},
        Obstacle{Vector2{1600.0f, 250.0f}, 38.0f, ObstacleKind::Rock},
        Obstacle{Vector2{950.0f, 180.0f}, 34.0f, ObstacleKind::Cactus},
    };
    room0.lootSpawns = {LootSpawn{Vector2{700.0f, 600.0f}, WeaponType::NachoShield}};

    Room room1;
    room1.waves = {
        {{EnemyType::PickleSplitter, 4}, {EnemyType::SodaBomber, 2}},
        {{EnemyType::CheddarShooter, 3}, {EnemyType::TwistCharger, 2, /*eliteEvery=*/2}},
    };
    room1.obstacles = {
        Obstacle{Vector2{950.0f, 525.0f}, 40.0f, ObstacleKind::Rock},
        Obstacle{Vector2{300.0f, 900.0f}, 34.0f, ObstacleKind::Cactus},
        Obstacle{Vector2{1650.0f, 150.0f}, 34.0f, ObstacleKind::Cactus},
    };
    room1.lootSpawns = {LootSpawn{Vector2{700.0f, 700.0f}, WeaponType::SalsaScattershot}};

    Room boss;
    boss.isBossRoom = true;
    boss.bossType = EnemyType::BossBurrower;

    return BuildLevel("Level 2: The Scorched Sandwich Flats", Biome::Desert, {room0, room1, boss});
}

// --- Level 3: Lava ----------------------------------------------------------
Level MakeLevel3() {
    Room room0;
    room0.waves = {
        {{EnemyType::GlazedChaser, 7}, {EnemyType::CheddarShooter, 3}},
        {{EnemyType::TwistCharger, 2}, {EnemyType::SodaBomber, 3}},
    };
    room0.hazards.push_back(Hazard{Vector2{cfg::kRoomWidth * 0.5f, cfg::kRoomHeight * 0.5f}, 70.0f});
    room0.obstacles = {
        Obstacle{Vector2{350.0f, 250.0f}, 36.0f, ObstacleKind::Rock},
        Obstacle{Vector2{1600.0f, 800.0f}, 38.0f, ObstacleKind::Rock},
    };
    room0.lootSpawns = {LootSpawn{Vector2{700.0f, 700.0f}, WeaponType::BurritoBomb}};

    Room room1;
    room1.waves = {
        {{EnemyType::GlazedChaser, 8, /*eliteEvery=*/3}, {EnemyType::TwistCharger, 2}},
        {{EnemyType::PickleSplitter, 3}, {EnemyType::SodaBomber, 3}, {EnemyType::CheddarShooter, 2}},
    };
    room1.hazards.push_back(Hazard{Vector2{cfg::kRoomWidth * 0.3f, cfg::kRoomHeight * 0.25f}, 60.0f});
    room1.hazards.push_back(Hazard{Vector2{cfg::kRoomWidth * 0.7f, cfg::kRoomHeight * 0.75f}, 60.0f});
    room1.obstacles = {
        Obstacle{Vector2{950.0f, 525.0f}, 42.0f, ObstacleKind::Rock},
        Obstacle{Vector2{1650.0f, 150.0f}, 34.0f, ObstacleKind::Rock},
    };
    room1.lootSpawns = {LootSpawn{Vector2{500.0f, 500.0f}, WeaponType::HabaneroHandful}};

    Room boss;
    boss.isBossRoom = true;
    boss.bossType = EnemyType::BossCaster;

    return BuildLevel("Level 3: The Molten Fondue Core", Biome::Lava, {room0, room1, boss});
}

// --- Level 4: Ice (final) ---------------------------------------------------
Level MakeLevel4() {
    Room room0;
    room0.waves = {
        {{EnemyType::GlazedChaser, 8, /*eliteEvery=*/3}, {EnemyType::CheddarShooter, 3}},
        {{EnemyType::PickleSplitter, 4}, {EnemyType::TwistCharger, 2}},
    };
    room0.obstacles = {
        Obstacle{Vector2{450.0f, 250.0f}, 36.0f, ObstacleKind::IceCrystal},
        Obstacle{Vector2{1500.0f, 800.0f}, 36.0f, ObstacleKind::IceCrystal},
        Obstacle{Vector2{950.0f, 525.0f}, 30.0f, ObstacleKind::IceCrystal},
    };
    room0.lootSpawns = {LootSpawn{Vector2{700.0f, 700.0f}, WeaponType::FondueFork}};

    Room room1;
    room1.waves = {
        {{EnemyType::SodaBomber, 4}, {EnemyType::CheddarShooter, 3}},
        {{EnemyType::TwistCharger, 2, /*eliteEvery=*/2}, {EnemyType::PickleSplitter, 3}, {EnemyType::GlazedChaser, 4}},
    };
    room1.obstacles = {
        Obstacle{Vector2{350.0f, 800.0f}, 34.0f, ObstacleKind::IceCrystal},
        Obstacle{Vector2{1600.0f, 250.0f}, 34.0f, ObstacleKind::IceCrystal},
    };
    room1.lootSpawns = {LootSpawn{Vector2{900.0f, 700.0f}, WeaponType::NachoShield}};

    Room boss;
    boss.isBossRoom = true;
    boss.bossType = EnemyType::BossFrost;

    return BuildLevel("Level 4: The Frozen Dessert Tundra", Biome::Ice, {room0, room1, boss});
}

}  // namespace

Level MakeLevel(int index) {
    switch (index) {
        case 0: return MakeLevel1();
        case 1: return MakeLevel2();
        case 2: return MakeLevel3();
        case 3: return MakeLevel4();
        default: return MakeLevel1();
    }
}
