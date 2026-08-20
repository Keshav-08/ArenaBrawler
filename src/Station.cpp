#include "Station.hpp"

std::string WallBuy::PromptText(ZombieWeaponKind currentWeapon) const {
    ZombieWeaponStats stats = GetZombieWeaponStats(weapon);
    if (currentWeapon == weapon) {
        if (stats.magazineSize <= 0) return std::string("Already equipped: ") + stats.name;
        return "[E] Refill Ammo - Cost: " + std::to_string(stats.ammoRefillCost);
    }
    return "[E] Buy " + std::string(stats.name) + " - Cost: " + std::to_string(stats.wallBuyCost);
}

WallBuyResult WallBuy::Interact(ZombieWeaponKind currentWeapon, Economy& economy) const {
    ZombieWeaponStats stats = GetZombieWeaponStats(weapon);
    if (currentWeapon == weapon) {
        if (stats.magazineSize <= 0) return WallBuyResult::None; // nothing to refill (melee/no-ammo weapon)
        if (!economy.Spend(stats.ammoRefillCost)) return WallBuyResult::None;
        return WallBuyResult::Refilled;
    }
    if (!economy.Spend(stats.wallBuyCost)) return WallBuyResult::None;
    return WallBuyResult::Purchased;
}

void MysteryBox::Update(float dt) {
    if (state_ == State::Cycling) {
        timer_ -= dt;
        flickerTimer_ -= dt;
        if (flickerTimer_ <= 0.0f) {
            flickerTimer_ = 0.08f;
            displayIndex_ = GetRandomValue(0, static_cast<int>(ZombieWeaponKind::Count) - 1);
        }
        if (timer_ <= 0.0f) {
            resultWeapon_ = static_cast<ZombieWeaponKind>(GetRandomValue(0, static_cast<int>(ZombieWeaponKind::Count) - 1));
            state_ = State::Ready;
            timer_ = kReadyDuration;
        }
    } else if (state_ == State::Ready) {
        timer_ -= dt;
        if (timer_ <= 0.0f) state_ = State::Idle;
    }
}

bool MysteryBox::TryActivate(Vector2 playerPos, Economy& economy, bool powered) {
    if (state_ != State::Idle || !InRange(playerPos) || !powered) return false;
    if (!economy.Spend(kCost)) return false;
    state_ = State::Cycling;
    timer_ = kCycleDuration;
    flickerTimer_ = 0.0f;
    return true;
}

bool MysteryBox::TryTakeWeapon(Vector2 playerPos, ZombieWeaponKind& outWeapon) {
    if (state_ != State::Ready || !InRange(playerPos)) return false;
    outWeapon = resultWeapon_;
    state_ = State::Idle;
    return true;
}

std::string MysteryBox::PromptText(Vector2 playerPos, bool powered) const {
    if (!InRange(playerPos)) return "";
    switch (state_) {
        case State::Idle:
            if (!powered) return "The Blender is dark - Requires Power";
            return "[E] Activate The Blender - Cost: " + std::to_string(kCost);
        case State::Ready: return "[E] Take " + std::string(GetZombieWeaponStats(resultWeapon_).name);
        default: return "The Blender is cycling...";
    }
}

std::string PowerSwitch::PromptText() const {
    if (activated) return "";
    return "[E] Restore Power - Cost: " + std::to_string(cost);
}

bool PowerSwitch::TryActivate(Economy& economy) {
    if (activated) return false;
    if (!economy.Spend(cost)) return false;
    activated = true;
    return true;
}

const char* PerkName(PerkKind kind) {
    switch (kind) {
        case PerkKind::Juggernog: return "Jalapeno Juggernog";
        case PerkKind::SpeedyReload: return "Speedy Sauce";
        case PerkKind::DoubleDamage: return "Double Scoop";
        case PerkKind::IronStomach: return "Iron Stomach";
        default: return "";
    }
}

std::string PerkMachine::PromptText(bool powered) const {
    if (purchased) return std::string(PerkName(kind)) + " (owned)";
    if (!powered) return std::string("[E] ") + PerkName(kind) + " - Requires Power";
    return "[E] Buy " + std::string(PerkName(kind)) + " - Cost: " + std::to_string(cost);
}

bool PerkMachine::TryPurchase(Economy& economy, bool powered) {
    if (purchased || !powered) return false;
    if (!economy.Spend(cost)) return false;
    purchased = true;
    return true;
}

void MysteryBox::Draw() const {
    Color boxColor = state_ == State::Ready ? GOLD : Color{90, 70, 140, 255};
    Rectangle box{position.x - 26.0f, position.y - 26.0f, 52.0f, 52.0f};
    DrawRectangleRec(box, boxColor);
    DrawRectangleLinesEx(box, 3.0f, Fade(BLACK, 0.6f));

    if (state_ == State::Cycling) {
        const char* name = GetZombieWeaponStats(static_cast<ZombieWeaponKind>(displayIndex_)).name;
        int w = MeasureText(name, 14);
        DrawText(name, static_cast<int>(position.x) - w / 2, static_cast<int>(position.y) - 40, 14, YELLOW);
    } else if (state_ == State::Ready) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 8.0f);
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 34.0f + pulse * 4.0f, Fade(GOLD, 0.7f));
    }
}
