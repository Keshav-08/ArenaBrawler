#pragma once

#include "Common.hpp"
#include "Economy.hpp"
#include "ZombieWeapon.hpp"
#include <string>

constexpr float kStationInteractRange = 60.0f;

enum class WallBuyResult { None, Purchased, Refilled };

// Fixed interactable on a wall: buys a specific weapon (if not already
// equipped) or refills its ammo (if it is).
struct WallBuy {
    Vector2 position{};
    ZombieWeaponKind weapon;

    bool InRange(Vector2 pos) const { return Vector2Distance(pos, position) <= kStationInteractRange; }
    std::string PromptText(ZombieWeaponKind currentWeapon) const;

    // Spends from `economy` and reports what happened; caller applies the
    // effect (equip fresh weapon vs. top off ammo) using the same stats
    // lookup (GetZombieWeaponStats) since this struct stays plain data.
    WallBuyResult Interact(ZombieWeaponKind currentWeapon, Economy& economy) const;
};

// "The Blender": costs 950 to spin up a ~2.5s cycling animation, then holds
// a random weapon ready to take for 5s before closing back up.
class MysteryBox {
public:
    Vector2 position{};

    void Update(float dt);
    void Draw() const;
    bool InRange(Vector2 pos) const { return Vector2Distance(pos, position) <= kStationInteractRange; }
    std::string PromptText(Vector2 playerPos, bool powered) const;

    bool TryActivate(Vector2 playerPos, Economy& economy, bool powered);
    bool TryTakeWeapon(Vector2 playerPos, ZombieWeaponKind& outWeapon);

private:
    enum class State { Idle, Cycling, Ready };
    State state_ = State::Idle;
    float timer_ = 0.0f;
    float flickerTimer_ = 0.0f;
    int displayIndex_ = 0;
    ZombieWeaponKind resultWeapon_ = ZombieWeaponKind::Blaster;

    static constexpr int kCost = 950;
    static constexpr float kCycleDuration = 2.5f;
    static constexpr float kReadyDuration = 5.0f;
};

// One-shot objective, modeled on Barrier's cost/cleared shape rather than a
// new state machine: unlocks the Mystery Box and perk machines (real CoD
// Zombies convention — wall-buys work without power, those two don't).
struct PowerSwitch {
    Vector2 position{};
    int cost = 0;
    bool activated = false;

    bool InRange(Vector2 pos) const { return Vector2Distance(pos, position) <= kStationInteractRange; }
    std::string PromptText() const;
    bool TryActivate(Economy& economy);
};

enum class PerkKind { Juggernog, SpeedyReload, DoubleDamage, IronStomach };
const char* PerkName(PerkKind kind);

// Permanent-for-the-run stat buff, bought once. Unlike WallBuy (repeatably
// purchasable), a perk needs its own "already bought" state, so this stays
// closer to MysteryBox's statefulness — but stays plain data like WallBuy:
// the caller applies the actual stat effect (see ZombiesMode::HandleInteract),
// since which field a PerkKind touches (Player vs. ZombiesMode-owned state)
// varies per perk.
struct PerkMachine {
    Vector2 position{};
    PerkKind kind = PerkKind::Juggernog;
    int cost = 0;
    bool purchased = false;

    bool InRange(Vector2 pos) const { return Vector2Distance(pos, position) <= kStationInteractRange; }
    std::string PromptText(bool powered) const;
    // Spends from `economy` and marks purchased; requires `powered` (the
    // PowerSwitch above) and refuses a second purchase.
    bool TryPurchase(Economy& economy, bool powered);
};
