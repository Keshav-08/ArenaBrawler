#pragma once

#include "Room.hpp"
#include <string>
#include <vector>

// A level is an ordered sequence of rooms laid out left-to-right in world
// space; the last room is always the boss room. `MakeLevel` holds the
// concrete level data (same factory-function pattern as Enemy.hpp's
// MakeEnemy) so adding a level later is just adding another case there.
struct Level {
    std::string name;
    Biome biome = Biome::Grass;
    std::vector<Room> rooms;
};

constexpr int kLevelCount = 4;

// Builds level `index` (0-based) with world-space room bounds already laid
// out sequentially along X, starting at x = 0.
Level MakeLevel(int index);
