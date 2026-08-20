#include "FloatingText.hpp"

FloatingTextEntry* FloatingTextPool::AcquireSlot() {
    for (auto& e : entries_) {
        if (!e.active) return &e;
    }
    return nullptr; // pool exhausted; the hit still lands, just no popup
}

void FloatingTextPool::Spawn(Vector2 pos, const std::string& text, Color color) {
    FloatingTextEntry* slot = AcquireSlot();
    if (!slot) return;
    slot->position = pos;
    slot->life = kFloatingTextLife;
    slot->text = text;
    slot->color = color;
    slot->active = true;
}

void FloatingTextPool::Update(float dt) {
    for (auto& e : entries_) {
        if (!e.active) continue;
        e.life -= dt;
        e.position.y -= kFloatingTextDrift * dt;
        if (e.life <= 0.0f) e.active = false;
    }
}

void FloatingTextPool::Draw() const {
    for (const auto& e : entries_) {
        if (!e.active) continue;
        float alpha = mathutil::Clamp01(e.life / kFloatingTextLife);
        int fontSize = 15;
        int w = MeasureText(e.text.c_str(), fontSize);
        DrawText(e.text.c_str(), static_cast<int>(e.position.x) - w / 2, static_cast<int>(e.position.y), fontSize,
                 ColorAlpha(e.color, alpha));
    }
}
