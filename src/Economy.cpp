#include "Economy.hpp"

EconomyPopup* Economy::AcquireSlot() {
    for (auto& p : popups_) {
        if (!p.active) return &p;
    }
    return nullptr; // pool exhausted; the points still land, just no popup
}

void Economy::AddPoints(int amount, Vector2 worldPos) {
    points_ += amount;
    EconomyPopup* slot = AcquireSlot();
    if (!slot) return;
    slot->position = worldPos;
    slot->life = kEconomyPopupLife;
    slot->text = "+" + std::to_string(amount);
    slot->color = amount >= 100 ? GOLD : RAYWHITE;
    slot->active = true;
}

bool Economy::Spend(int amount) {
    if (points_ < amount) return false;
    points_ -= amount;
    return true;
}

void Economy::Update(float dt) {
    for (auto& p : popups_) {
        if (!p.active) continue;
        p.life -= dt;
        p.position.y -= kEconomyPopupDrift * dt;
        if (p.life <= 0.0f) p.active = false;
    }
}

void Economy::Draw() const {
    for (const auto& p : popups_) {
        if (!p.active) continue;
        float alpha = mathutil::Clamp01(p.life / kEconomyPopupLife);
        int fontSize = 16;
        int w = MeasureText(p.text.c_str(), fontSize);
        DrawText(p.text.c_str(), static_cast<int>(p.position.x) - w / 2, static_cast<int>(p.position.y), fontSize,
                 ColorAlpha(p.color, alpha));
    }
}
