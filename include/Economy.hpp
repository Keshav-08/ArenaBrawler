#pragma once

#include "Common.hpp"
#include <array>
#include <string>

constexpr int kMaxEconomyPopups = 32;
constexpr float kEconomyPopupLife = 0.8f;
constexpr float kEconomyPopupDrift = 46.0f; // px/sec upward

struct EconomyPopup {
    Vector2 position{};
    float life = 0.0f;
    std::string text;
    Color color = WHITE;
    bool active = false;
};

// Zombies Mode's standalone point economy ("Crumbs") — intentionally
// separate from StoryMode's ComboTracker/score so the two modes never share
// scoring state. Owns a fixed-capacity pool of floating "+N" popups, same
// no-per-frame-allocation pattern as ParticleSystem/PickupManager.
class Economy {
public:
    void AddPoints(int amount, Vector2 worldPos);
    bool Spend(int amount);
    int Points() const { return points_; }

    void Update(float dt);
    void Draw() const;

private:
    EconomyPopup* AcquireSlot();

    int points_ = 0;
    std::array<EconomyPopup, kMaxEconomyPopups> popups_{};
};
