#pragma once

#include "Common.hpp"
#include "Room.hpp"

// Shared obstacle-shape rendering, used by both StoryMode's outdoor terrain
// (rocks/trees/cacti/ice) and ZombiesMode's indoor set-dressing
// (tables/shelves/counters/meat racks) — pure rendering, no dependency on
// either mode's state, so both can call the same function instead of
// duplicating ~50 lines of shape-drawing code.
namespace fx {
void DrawObstacle(const Obstacle& obs, Vector2 center);
}
