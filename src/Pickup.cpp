#include "Pickup.hpp"

Pickup* PickupManager::AcquireSlot() {
    for (auto& p : pool_) {
        if (!p.active) return &p;
    }
    return nullptr; // pool exhausted; drop is silently skipped
}

void PickupManager::SpawnHealth(Vector2 pos) {
    Pickup* slot = AcquireSlot();
    if (!slot) return;
    *slot = Pickup{};
    slot->position = pos;
    slot->life = cfg::kPickupLife;
    slot->kind = PickupKind::Health;
    slot->active = true;
}

void PickupManager::SpawnPowerUp(Vector2 pos, PowerUpType type) {
    Pickup* slot = AcquireSlot();
    if (!slot) return;
    *slot = Pickup{};
    slot->position = pos;
    slot->life = cfg::kPickupLife;
    slot->kind = PickupKind::PowerUp;
    slot->powerUpType = type;
    slot->active = true;
}

void PickupManager::SpawnWeaponLoot(Vector2 pos, WeaponType type) {
    Pickup* slot = AcquireSlot();
    if (!slot) return;
    *slot = Pickup{};
    slot->position = pos;
    slot->life = -1.0f; // placed loot never despawns, unlike kill drops
    slot->kind = PickupKind::WeaponLoot;
    slot->weaponLootType = type;
    slot->active = true;
}

void PickupManager::SpawnAmmo(Vector2 pos) {
    Pickup* slot = AcquireSlot();
    if (!slot) return;
    *slot = Pickup{};
    slot->position = pos;
    slot->life = cfg::kPickupLife;
    slot->kind = PickupKind::Ammo;
    slot->active = true;
}

void PickupManager::SpawnArmor(Vector2 pos, float reduction) {
    Pickup* slot = AcquireSlot();
    if (!slot) return;
    *slot = Pickup{};
    slot->position = pos;
    slot->life = cfg::kPickupLife;
    slot->kind = PickupKind::Armor;
    slot->armorReduction = reduction;
    slot->active = true;
}

void PickupManager::RollAndSpawnDrop(Vector2 pos) {
    if (mathutil::RandomFloat(0.0f, 1.0f) >= cfg::kPickupDropChance * dropChanceMult) return;
    float roll = mathutil::RandomFloat(0.0f, 1.0f);
    if (roll < cfg::kPowerUpDropShare) {
        int count = static_cast<int>(PowerUpType::Count);
        SpawnPowerUp(pos, static_cast<PowerUpType>(GetRandomValue(0, count - 1)));
    } else if (roll < cfg::kPowerUpDropShare + cfg::kAmmoDropShare) {
        SpawnAmmo(pos);
    } else if (roll < cfg::kPowerUpDropShare + cfg::kAmmoDropShare + cfg::kArmorDropShare) {
        float reduction = GetRandomValue(0, 1) == 0 ? cfg::kLightArmorReduction : cfg::kHeavyArmorReduction;
        SpawnArmor(pos, reduction);
    } else {
        SpawnHealth(pos);
    }
}

void PickupManager::Update(float dt) {
    for (auto& p : pool_) {
        if (!p.active) continue;
        if (p.life < 0.0f) continue; // placed loot: never despawns
        p.life -= dt;
        if (p.life <= 0.0f) p.active = false;
    }
}

void PickupManager::Draw() const {
    for (const auto& p : pool_) {
        if (!p.active) continue;
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 5.0f);

        if (p.kind == PickupKind::Health) {
            DrawCircleV(p.position, cfg::kPickupRadius + pulse, Color{80, 220, 110, 255});
            DrawCircleLines(static_cast<int>(p.position.x), static_cast<int>(p.position.y),
                             cfg::kPickupRadius + pulse, Fade(WHITE, 0.6f));
            DrawLineEx(Vector2{p.position.x - 4.0f, p.position.y}, Vector2{p.position.x + 4.0f, p.position.y}, 2.0f, WHITE);
            DrawLineEx(Vector2{p.position.x, p.position.y - 4.0f}, Vector2{p.position.x, p.position.y + 4.0f}, 2.0f, WHITE);
        } else if (p.kind == PickupKind::Ammo) {
            Rectangle box{p.position.x - 9.0f, p.position.y - 7.0f, 18.0f, 14.0f};
            DrawRectangleRec(box, Color{90, 150, 90, 255});
            DrawRectangleLinesEx(box, 2.0f, Color{40, 80, 40, 255});
            DrawCircleV(p.position, 3.0f + pulse * 0.5f, Color{220, 230, 200, 255});
        } else if (p.kind == PickupKind::Armor) {
            Color c = p.armorReduction >= cfg::kHeavyArmorReduction ? Color{160, 170, 200, 255} : Color{140, 150, 170, 255};
            Vector2 top{p.position.x, p.position.y - 12.0f};
            Vector2 left{p.position.x - 10.0f, p.position.y - 2.0f};
            Vector2 right{p.position.x + 10.0f, p.position.y - 2.0f};
            Vector2 bottom{p.position.x, p.position.y + 12.0f};
            DrawTriangle(top, left, bottom, c);
            DrawTriangle(top, bottom, right, c);
            DrawTriangleLines(top, left, bottom, Fade(BLACK, 0.5f));
            DrawTriangleLines(top, bottom, right, Fade(BLACK, 0.5f));
        } else if (p.kind == PickupKind::WeaponLoot) {
            Color c = WeaponTypeColor(p.weaponLootType);
            float r = cfg::kPickupRadius + 4.0f + pulse * 2.0f;
            DrawRectangle(static_cast<int>(p.position.x - r), static_cast<int>(p.position.y - r),
                          static_cast<int>(r * 2.0f), static_cast<int>(r * 2.0f), Fade(c, 0.35f));
            DrawRectangleLines(static_cast<int>(p.position.x - r), static_cast<int>(p.position.y - r),
                                static_cast<int>(r * 2.0f), static_cast<int>(r * 2.0f), c);
            const char* label = WeaponTypeLabel(p.weaponLootType);
            int fontSize = 12;
            int textW = MeasureText(label, fontSize);
            DrawRectangle(static_cast<int>(p.position.x) - textW / 2 - 4, static_cast<int>(p.position.y - r - 20),
                          textW + 8, fontSize + 4, Fade(BLACK, 0.6f));
            DrawText(label, static_cast<int>(p.position.x) - textW / 2, static_cast<int>(p.position.y - r - 18), fontSize, c);
        } else {
            Color c = PowerUpColor(p.powerUpType);
            float r = cfg::kPickupRadius + 3.0f + pulse * 2.5f;
            // Diamond marker + rotating ring so a power-up drop reads as
            // unmistakably rarer/more exciting than a plain health orb.
            Vector2 top{p.position.x, p.position.y - r};
            Vector2 right{p.position.x + r, p.position.y};
            Vector2 bottom{p.position.x, p.position.y + r};
            Vector2 left{p.position.x - r, p.position.y};
            DrawTriangle(top, left, bottom, c);
            DrawTriangle(top, bottom, right, c);
            DrawLineEx(top, right, 2.0f, WHITE);
            DrawLineEx(right, bottom, 2.0f, WHITE);
            DrawLineEx(bottom, left, 2.0f, WHITE);
            DrawLineEx(left, top, 2.0f, WHITE);
            DrawCircleLines(static_cast<int>(p.position.x), static_cast<int>(p.position.y), r + 6.0f, Fade(c, 0.5f));

            const char* label = PowerUpLabel(p.powerUpType);
            int fontSize = 12;
            int textW = MeasureText(label, fontSize);
            DrawRectangle(static_cast<int>(p.position.x) - textW / 2 - 4, static_cast<int>(p.position.y - r - 20),
                          textW + 8, fontSize + 4, Fade(BLACK, 0.6f));
            DrawText(label, static_cast<int>(p.position.x) - textW / 2, static_cast<int>(p.position.y - r - 18), fontSize, c);
        }
    }
}

std::vector<Pickup> PickupManager::CollectNear(Vector2 pos, float radius) {
    std::vector<Pickup> collected;
    for (auto& p : pool_) {
        if (!p.active) continue;
        if (Vector2Distance(p.position, pos) > radius + cfg::kPickupRadius) continue;
        collected.push_back(p);
        p.active = false;
    }
    return collected;
}
