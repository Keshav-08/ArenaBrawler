#pragma once

#include "Common.hpp"
#include <array>
#include <string>

constexpr int kMaxFloatingTexts = 48;
constexpr float kFloatingTextLife = 0.6f;
constexpr float kFloatingTextDrift = 60.0f; // px/sec upward

struct FloatingTextEntry {
    Vector2 position{};
    float life = 0.0f;
    std::string text;
    Color color = WHITE;
    bool active = false;
};

// Generalizes Economy's EconomyPopup pattern (fixed pool, spawn/drift/fade,
// no per-frame allocation) to arbitrary text instead of only "+N" Crumbs, so
// both StoryMode and ZombiesMode can pop a number at every hit, not just
// point pickups.
class FloatingTextPool {
public:
    void Spawn(Vector2 pos, const std::string& text, Color color);

    void Update(float dt);
    void Draw() const;

private:
    FloatingTextEntry* AcquireSlot();

    std::array<FloatingTextEntry, kMaxFloatingTexts> entries_{};
};
