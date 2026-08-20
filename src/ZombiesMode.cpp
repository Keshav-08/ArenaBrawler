#include "ZombiesMode.hpp"
#include "ObstacleRender.hpp"
#include <algorithm>
#include <string>
#include <utility>

namespace {

constexpr float kRegenDelay = 5.0f;   // seconds without damage before regen kicks in
constexpr float kRegenRate = 8.0f;    // hp/sec
constexpr float kMeleeSwingVisualDuration = 0.15f;
constexpr float kChainAcquireRange = 320.0f;
constexpr float kChainAcquireArcDeg = 45.0f;
constexpr float kChainDamageFalloff = 0.7f; // per jump
constexpr float kLaserBeamWidth = 6.0f;
constexpr float kExplosionImpulse = 2.0e6f;

// Auto-reload duration by fire mode (bigger/heavier weapons take longer).
// Not per-weapon data — keeps the 30-entry stats table focused on what
// actually varies gun-to-gun.
float ReloadTimeFor(FireMode mode) {
    switch (mode) {
        case FireMode::SemiAuto: return 1.2f;
        case FireMode::FullAuto: return 1.8f;
        case FireMode::Burst: return 1.6f;
        case FireMode::Shotgun: return 2.2f;
        case FireMode::Explosive: return 2.5f;
        case FireMode::Freeze: return 1.8f;
        default: return 1.5f;
    }
}

std::string ToRoman(int n) {
    if (n <= 0) return std::to_string(n);
    static const std::pair<int, const char*> table[] = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"}, {90, "XC"},
        {50, "L"}, {40, "XL"}, {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"}, {1, "I"},
    };
    std::string result;
    for (const auto& [value, symbol] : table) {
        while (n >= value) {
            result += symbol;
            n -= value;
        }
    }
    return result;
}

struct ZonePalette {
    Color floor, border;
};

// Keyed by position, not a general theme enum — there are always exactly
// these 4 rooms in this fixed order (see MapGraph::BuildDefaultMap).
ZonePalette GetZonePalette(int zoneIndex) {
    switch (zoneIndex) {
        case 0: return ZonePalette{Color{72, 52, 42, 255}, Color{150, 110, 80, 255}};   // Cafeteria: warm tan/red
        case 1: return ZonePalette{Color{48, 52, 58, 255}, Color{110, 120, 135, 255}};  // Storage Hall: industrial gray/blue
        case 2: return ZonePalette{Color{58, 58, 62, 255}, Color{170, 172, 178, 255}};  // Industrial Kitchen: steel
        case 3: return ZonePalette{Color{50, 68, 82, 255}, Color{190, 220, 235, 255}};  // Deep Freezer: icy blue/white
        default: return ZonePalette{Color{45, 42, 50, 255}, Color{95, 90, 100, 255}};
    }
}

void DrawZone(const Zone& zone, int zoneIndex) {
    ZonePalette palette = GetZonePalette(zoneIndex);
    DrawRectangleRec(zone.bounds, palette.floor);
    constexpr float kTile = 90.0f;
    Color grid = Fade(BLACK, 0.15f);
    for (float gx = zone.bounds.x; gx <= zone.bounds.x + zone.bounds.width; gx += kTile) {
        DrawLineV(Vector2{gx, zone.bounds.y}, Vector2{gx, zone.bounds.y + zone.bounds.height}, grid);
    }
    for (float gy = zone.bounds.y; gy <= zone.bounds.y + zone.bounds.height; gy += kTile) {
        DrawLineV(Vector2{zone.bounds.x, gy}, Vector2{zone.bounds.x + zone.bounds.width, gy}, grid);
    }
    DrawRectangleLinesEx(zone.bounds, 4.0f, palette.border);

    for (const Obstacle& obs : zone.obstacles) {
        Vector2 center{zone.bounds.x + obs.center.x, zone.bounds.y + obs.center.y};
        fx::DrawObstacle(obs, center);
    }

    int w = MeasureText(zone.name.c_str(), 24);
    DrawText(zone.name.c_str(), static_cast<int>(zone.bounds.x + zone.bounds.width * 0.5f) - w / 2,
              static_cast<int>(zone.bounds.y + 20.0f), 24, Fade(WHITE, 0.5f));
}

void DrawBarrier(const Barrier& b) {
    if (b.cleared) {
        DrawRectangleRec(b.bounds, Fade(LIME, 0.12f));
        return;
    }
    float pulse = 0.55f + 0.45f * std::sin(static_cast<float>(GetTime()) * 6.0f);
    DrawRectangleRec(b.bounds, ColorAlpha(RED, 0.35f + 0.25f * pulse));
    DrawRectangleLinesEx(b.bounds, 4.0f, ColorAlpha(ORANGE, 0.9f));
}

// Colors by fire-mode family (not individual gun) so the roster scales to
// 30 entries without a 30-case switch.
Color WeaponColorFor(ZombieWeaponKind kind) {
    switch (GetZombieWeaponStats(kind).fireMode) {
        case FireMode::Melee: return Color{140, 220, 90, 255};
        case FireMode::SemiAuto: return Color{230, 210, 120, 255};
        case FireMode::FullAuto: return Color{255, 200, 90, 255};
        case FireMode::Burst: return Color{255, 170, 110, 255};
        case FireMode::Shotgun: return Color{255, 140, 60, 255};
        case FireMode::Explosive: return Color{255, 100, 60, 255};
        case FireMode::Continuous: return Color{255, 100, 40, 255};
        case FireMode::Laser: return Color{255, 90, 220, 255};
        case FireMode::Chain: return Color{120, 200, 255, 255};
        case FireMode::Freeze: return Color{150, 220, 255, 255};
        default: return WHITE;
    }
}

void DrawWallBuy(const WallBuy& wb, ZombieWeaponKind equipped) {
    Color c = WeaponColorFor(wb.weapon);
    DrawRectangle(static_cast<int>(wb.position.x) - 16, static_cast<int>(wb.position.y) - 12, 32, 24, Fade(c, 0.5f));
    DrawRectangleLines(static_cast<int>(wb.position.x) - 16, static_cast<int>(wb.position.y) - 12, 32, 24, c);
    const char* name = GetZombieWeaponStats(wb.weapon).name;
    int w = MeasureText(name, 12);
    DrawText(name, static_cast<int>(wb.position.x) - w / 2, static_cast<int>(wb.position.y) - 32, 12,
             wb.weapon == equipped ? GOLD : WHITE);
}

void DrawHealthBar(Vector2 pos, float width, float height, float frac, Color fg) {
    DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width), static_cast<int>(height), Fade(DARKGRAY, 0.6f));
    DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width * mathutil::Clamp01(frac)), static_cast<int>(height), fg);
    DrawRectangleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width), static_cast<int>(height), RAYWHITE);
}

}  // namespace

ZombiesMode::ZombiesMode(AudioManager& audio) : audio_(audio) {}

void ZombiesMode::Enter() {
    map_.BuildDefaultMap();

    player_ = Player();
    player_.position = map_.StartSpawnPoint();
    player_.velocity = Vector2{0, 0};

    projectiles_ = ProjectileManager();
    particles_ = ParticleSystem();
    shake_ = ScreenShake();
    economy_ = Economy();
    zombies_.clear();
    pendingSplits_.clear();
    director_.StartRound();

    equippedWeapon_ = ZombieWeaponKind::Sword;
    ammo_ = 0;
    weaponCooldownTimer_ = 0.0f;
    swingTimer_ = 0.0f;
    burstActive_ = false;
    burstShotsRemaining_ = 0;
    reloading_ = false;
    reloadTimer_ = 0.0f;
    timeSinceLastHit_ = 0.0f;
    state_ = RunState::Playing;

    wallBuys_.clear();
    const Zone& cafeteria = map_.Zones()[0];
    wallBuys_.push_back(WallBuy{Vector2{cafeteria.bounds.x + cafeteria.bounds.width * 0.5f, cafeteria.bounds.y + 70.0f},
                                 ZombieWeaponKind::Blaster});
    const Zone& storage = map_.Zones()[1];
    wallBuys_.push_back(WallBuy{Vector2{storage.bounds.x + storage.bounds.width * 0.5f, storage.bounds.y + 70.0f},
                                 ZombieWeaponKind::NachoNinja});
    const Zone& kitchen = map_.Zones()[2];
    wallBuys_.push_back(WallBuy{Vector2{kitchen.bounds.x + kitchen.bounds.width * 0.5f, kitchen.bounds.y + 70.0f},
                                 ZombieWeaponKind::BaguetteBattleRifle});
    const Zone& freezer = map_.Zones()[3];
    wallBuys_.push_back(WallBuy{Vector2{freezer.bounds.x + freezer.bounds.width * 0.5f, freezer.bounds.y + 70.0f},
                                 ZombieWeaponKind::LongRibRifle});

    mysteryBox_ = MysteryBox();
    mysteryBox_.position = Vector2{freezer.bounds.x + freezer.bounds.width * 0.5f, freezer.bounds.y + freezer.bounds.height * 0.5f};
}

Camera2D ZombiesMode::BuildCamera() const {
    int zoneIdx = map_.ZoneIndexContaining(player_.position);
    Rectangle zone = map_.CameraBoundsFor(zoneIdx);
    Camera2D camera{};
    camera.offset = Vector2Add(Vector2{cfg::kScreenWidth / 2.0f, cfg::kScreenHeight / 2.0f}, shake_.Offset());
    camera.zoom = 1.0f;
    camera.rotation = 0.0f;

    float halfW = cfg::kScreenWidth / 2.0f;
    float halfH = cfg::kScreenHeight / 2.0f;
    float targetX = zone.width <= cfg::kScreenWidth
                         ? zone.x + zone.width * 0.5f
                         : Clamp(player_.position.x, zone.x + halfW, zone.x + zone.width - halfW);
    float targetY = zone.height <= cfg::kScreenHeight
                         ? zone.y + zone.height * 0.5f
                         : Clamp(player_.position.y, zone.y + halfH, zone.y + zone.height - halfH);
    camera.target = Vector2{targetX, targetY};
    return camera;
}

void ZombiesMode::HandleInteract() {
    if (!IsKeyPressed(KEY_E)) return;

    Barrier* barrier = map_.NearbyUnclearedBarrier(player_.position, kStationInteractRange);
    if (barrier) {
        if (map_.TryClearBarrier(*barrier, economy_)) {
            audio_.Play(Sfx::GateUnlock);
            shake_.Trigger(0.3f, 8.0f);
        }
        return;
    }

    for (WallBuy& wb : wallBuys_) {
        if (!wb.InRange(player_.position)) continue;
        WallBuyResult result = wb.Interact(equippedWeapon_, economy_);
        if (result == WallBuyResult::Purchased) {
            equippedWeapon_ = wb.weapon;
            ammo_ = GetZombieWeaponStats(wb.weapon).magazineSize;
            burstActive_ = false;
            reloading_ = false;
            audio_.Play(Sfx::PickupPowerUp);
        } else if (result == WallBuyResult::Refilled) {
            ammo_ = GetZombieWeaponStats(equippedWeapon_).magazineSize;
            reloading_ = false;
            audio_.Play(Sfx::PickupHealth);
        }
        return;
    }

    if (mysteryBox_.InRange(player_.position)) {
        ZombieWeaponKind taken;
        if (mysteryBox_.TryTakeWeapon(player_.position, taken)) {
            equippedWeapon_ = taken;
            ammo_ = GetZombieWeaponStats(taken).magazineSize;
            burstActive_ = false;
            reloading_ = false;
            audio_.Play(Sfx::PickupPowerUp, 1.0f, -0.15f);
            particles_.SpawnBurst(mysteryBox_.position, 24, GOLD, 100.0f, 300.0f, 0.3f, 0.6f);
        } else if (mysteryBox_.TryActivate(player_.position, economy_)) {
            audio_.Play(Sfx::GateUnlock, 1.0f, 0.2f);
        }
    }
}

void ZombiesMode::AwardKill(Enemy& zombie, int basePoints) {
    economy_.AddPoints(basePoints, zombie.position);
    particles_.SpawnBurst(zombie.position, 14, Color{200, 40, 40, 255}, 60.0f, 260.0f, 0.25f, 0.5f);
    audio_.Play(Sfx::EnemyDeath, 0.7f, 0.15f);
    if (zombie.SplitsOnDeath()) {
        Vector2 offset = mathutil::FromAngle(mathutil::RandomFloat(0.0f, 2.0f * PI), cfg::kPickleSplitterSplitOffset);
        pendingSplits_.emplace_back(EnemyType::PickleSplitter, Vector2Add(zombie.position, offset));
        pendingSplits_.emplace_back(EnemyType::PickleSplitter, Vector2Subtract(zombie.position, offset));
    }
}

// ---------------------------------------------------------------------------
// Fire-mode resolvers — HandleAttack dispatches into these based on
// GetZombieWeaponStats(equippedWeapon_).fireMode, so most of the 30 guns
// need zero new code, just a stats row (see ZombieWeapon.hpp).
// ---------------------------------------------------------------------------

void ZombiesMode::FireMelee(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats) {
    weaponCooldownTimer_ = stats.cooldown;
    swingTimer_ = kMeleeSwingVisualDuration;
    swingAngle_ = mathutil::AngleOf(aimDir);
    audio_.Play(Sfx::SwordSwing, 0.7f, 0.2f);

    float halfArc = mathutil::DegToRadF(stats.arcDeg) * 0.5f;
    bool hitAnything = false;
    for (auto& z : zombies_) {
        if (!z->IsAlive()) continue;
        Vector2 toZ = Vector2Subtract(z->position, origin);
        float dist = Vector2Length(toZ);
        if (dist > stats.range + z->radius || dist < 0.0001f) continue;
        if (std::fabs(mathutil::AngleDiff(swingAngle_, mathutil::AngleOf(toZ))) > halfArc) continue;

        hitAnything = true;
        bool killed = z->TakeDamage(stats.damage);
        Vector2 pushDir = Vector2Scale(toZ, 1.0f / dist);
        z->velocity = Vector2Add(z->velocity, Vector2Scale(pushDir, 500.0f));
        economy_.AddPoints(10, z->position);
        particles_.SpawnBurst(z->position, 10, Color{240, 240, 240, 255}, 80.0f, 260.0f, 0.15f, 0.35f);
        if (killed) AwardKill(*z, 130);
    }
    if (hitAnything) shake_.Trigger(0.15f, 5.0f);
}

void ZombiesMode::FireBulletShot(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats) {
    weaponCooldownTimer_ = stats.cooldown;
    float angle = mathutil::AngleOf(aimDir) + mathutil::DegToRadF(mathutil::RandomFloat(-stats.spreadDeg, stats.spreadDeg));
    Vector2 dir = mathutil::FromAngle(angle);
    Vector2 vel = Vector2Scale(dir, stats.bulletSpeed);
    Vector2 spawnPos = Vector2Add(origin, Vector2Scale(dir, cfg::kPlayerRadius + 4.0f));
    projectiles_.SpawnBullet(spawnPos, vel, stats.damage, 4.0f, 1.5f, false, stats.slowDuration);
    particles_.SpawnMuzzleFlash(spawnPos, angle, Color{255, 200, 120, 255});
    audio_.Play(Sfx::BlasterShot, 0.5f, 0.15f);
    if (stats.magazineSize > 0) {
        ammo_--;
        if (ammo_ <= 0) StartReload(stats);
    }
}

void ZombiesMode::FireBurst(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats) {
    weaponCooldownTimer_ = stats.cooldown;
    burstActive_ = true;
    burstShotsRemaining_ = stats.burstCount;
    burstTimer_ = 0.0f; // fires the first round immediately via UpdateBurst this same frame
    burstOrigin_ = origin;
    burstAimDir_ = aimDir;
}

void ZombiesMode::UpdateBurst(float dt) {
    if (!burstActive_) return;
    burstTimer_ -= dt;
    if (burstTimer_ > 0.0f) return;

    const ZombieWeaponStats& stats = GetZombieWeaponStats(equippedWeapon_);
    if (stats.fireMode != FireMode::Burst || (stats.magazineSize > 0 && ammo_ <= 0)) {
        burstActive_ = false;
        return;
    }
    FireBulletShot(burstOrigin_, burstAimDir_, stats);
    burstShotsRemaining_--;
    burstTimer_ = stats.burstDelay;
    if (burstShotsRemaining_ <= 0) burstActive_ = false;
}

void ZombiesMode::StartReload(const ZombieWeaponStats& stats) {
    if (stats.magazineSize <= 0 || reloading_) return;
    reloading_ = true;
    reloadTimer_ = ReloadTimeFor(stats.fireMode);
}

void ZombiesMode::UpdateReload(float dt) {
    if (!reloading_) return;
    reloadTimer_ -= dt;
    if (reloadTimer_ <= 0.0f) {
        reloading_ = false;
        ammo_ = GetZombieWeaponStats(equippedWeapon_).magazineSize;
    }
}

void ZombiesMode::FireShotgun(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats) {
    weaponCooldownTimer_ = stats.cooldown;
    float baseAngle = mathutil::AngleOf(aimDir);
    for (int i = 0; i < stats.pelletCount; ++i) {
        float angle = baseAngle + mathutil::DegToRadF(mathutil::RandomFloat(-stats.spreadDeg, stats.spreadDeg));
        Vector2 dir = mathutil::FromAngle(angle);
        Vector2 vel = Vector2Scale(dir, stats.bulletSpeed);
        Vector2 spawnPos = Vector2Add(origin, Vector2Scale(dir, cfg::kPlayerRadius + 4.0f));
        projectiles_.SpawnBullet(spawnPos, vel, stats.damage, 4.0f, 0.4f);
    }
    particles_.SpawnMuzzleFlash(origin, baseAngle, Color{255, 200, 120, 255});
    audio_.Play(Sfx::BlasterShot, 0.7f, -0.1f);
    if (stats.magazineSize > 0) {
        ammo_--;
        if (ammo_ <= 0) StartReload(stats);
    }
}

void ZombiesMode::FireExplosive(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats) {
    weaponCooldownTimer_ = stats.cooldown;
    Vector2 vel = Vector2Scale(aimDir, stats.bulletSpeed);
    projectiles_.SpawnBomb(origin, vel, stats.damage, stats.blastRadius, 1.2f);
    if (stats.magazineSize > 0) {
        ammo_--;
        if (ammo_ <= 0) StartReload(stats);
    }
}

void ZombiesMode::FireContinuousCone(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats, float dt) {
    float halfArc = mathutil::DegToRadF(stats.arcDeg) * 0.5f;
    float aimAngle = mathutil::AngleOf(aimDir);
    for (auto& z : zombies_) {
        if (!z->IsAlive()) continue;
        Vector2 toZ = Vector2Subtract(z->position, origin);
        float dist = Vector2Length(toZ);
        if (dist > stats.range + z->radius || dist < 0.0001f) continue;
        if (std::fabs(mathutil::AngleDiff(aimAngle, mathutil::AngleOf(toZ))) > halfArc) continue;

        bool killed = z->TakeDamage(stats.damage * dt);
        economy_.AddPoints(10, z->position);
        if (killed) AwardKill(*z, 130);
    }
    Vector2 flamePos = Vector2Add(origin, mathutil::FromAngle(aimAngle, 45.0f));
    particles_.SpawnBurst(flamePos, 3, Color{255, 140, 40, 255}, 60.0f, 160.0f, 0.08f, 0.2f);
}

void ZombiesMode::FireLaser(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats, float dt) {
    Vector2 end = Vector2Add(origin, Vector2Scale(aimDir, stats.range));
    Vector2 seg = Vector2Subtract(end, origin);
    float segLenSq = Vector2LengthSqr(seg);

    for (auto& z : zombies_) {
        if (!z->IsAlive()) continue;
        Vector2 toZ = Vector2Subtract(z->position, origin);
        float t = segLenSq > 0.0001f ? mathutil::Clamp01(Vector2DotProduct(toZ, seg) / segLenSq) : 0.0f;
        Vector2 closest = Vector2Add(origin, Vector2Scale(seg, t));
        if (Vector2Distance(z->position, closest) > z->radius + kLaserBeamWidth) continue;

        bool killed = z->TakeDamage(stats.damage * dt);
        economy_.AddPoints(10, z->position);
        if (killed) AwardKill(*z, 100);
    }
    particles_.SpawnBurst(Vector2Add(origin, Vector2Scale(aimDir, 30.0f)), 2, Color{255, 120, 220, 255}, 40.0f, 100.0f, 0.06f, 0.12f);
}

void ZombiesMode::FireChain(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats) {
    weaponCooldownTimer_ = stats.cooldown;
    float aimAngle = mathutil::AngleOf(aimDir);

    Enemy* target = nullptr;
    float bestDist = kChainAcquireRange;
    for (auto& z : zombies_) {
        if (!z->IsAlive()) continue;
        Vector2 toZ = Vector2Subtract(z->position, origin);
        float dist = Vector2Length(toZ);
        if (dist > bestDist || dist < 0.0001f) continue;
        if (std::fabs(mathutil::AngleDiff(aimAngle, mathutil::AngleOf(toZ))) > mathutil::DegToRadF(kChainAcquireArcDeg)) continue;
        target = z.get();
        bestDist = dist;
    }
    if (!target) return;

    audio_.Play(Sfx::BlasterShot, 0.6f, -0.15f);
    std::vector<Enemy*> hit;
    Enemy* current = target;
    float dmg = stats.damage;
    for (int jump = 0; jump < stats.chainCount && current != nullptr; ++jump) {
        bool killed = current->TakeDamage(dmg);
        economy_.AddPoints(10, current->position);
        particles_.SpawnBurst(current->position, 8, Color{160, 220, 255, 255}, 80.0f, 220.0f, 0.15f, 0.3f);
        if (killed) AwardKill(*current, 100);
        hit.push_back(current);

        Enemy* next = nullptr;
        float nextBest = stats.chainRadius;
        for (auto& z : zombies_) {
            if (!z->IsAlive() || std::find(hit.begin(), hit.end(), z.get()) != hit.end()) continue;
            float d = Vector2Distance(z->position, current->position);
            if (d < nextBest) {
                next = z.get();
                nextBest = d;
            }
        }
        current = next;
        dmg *= kChainDamageFalloff;
    }
}

void ZombiesMode::HandleAttack(Vector2 origin, Vector2 aimDir, bool heldNow, bool pressedNow, float dt) {
    UpdateBurst(dt);

    const ZombieWeaponStats& stats = GetZombieWeaponStats(equippedWeapon_);

    if (stats.fireMode == FireMode::Continuous) {
        if (heldNow) FireContinuousCone(origin, aimDir, stats, dt);
        return;
    }
    if (stats.fireMode == FireMode::Laser) {
        if (heldNow) FireLaser(origin, aimDir, stats, dt);
        return;
    }

    if (weaponCooldownTimer_ > 0.0f || burstActive_ || reloading_) return;
    if (stats.magazineSize > 0 && ammo_ <= 0) return;

    bool trigger = (stats.fireMode == FireMode::FullAuto || stats.fireMode == FireMode::Melee) ? heldNow : pressedNow;
    if (!trigger) return;

    switch (stats.fireMode) {
        case FireMode::Melee: FireMelee(origin, aimDir, stats); break;
        case FireMode::SemiAuto:
        case FireMode::FullAuto:
        case FireMode::Freeze:
            FireBulletShot(origin, aimDir, stats);
            break;
        case FireMode::Burst: FireBurst(origin, aimDir, stats); break;
        case FireMode::Shotgun: FireShotgun(origin, aimDir, stats); break;
        case FireMode::Explosive: FireExplosive(origin, aimDir, stats); break;
        case FireMode::Chain: FireChain(origin, aimDir, stats); break;
        default: break; // Continuous/Laser handled above
    }
}

void ZombiesMode::ResolveExplosion(const Explosion& ex, float dt) {
    particles_.SpawnBurst(ex.position, 40, Color{255, 170, 60, 255}, 120.0f, 480.0f, 0.3f, 0.7f, 3.0f, 7.0f);
    shake_.Trigger(0.35f, 12.0f);
    audio_.Play(Sfx::BombExplosion, 0.85f, 0.1f);

    for (auto& z : zombies_) {
        if (!z->IsAlive()) continue;
        Vector2 diff = Vector2Subtract(z->position, ex.position);
        float dist = Vector2Length(diff);
        if (dist > ex.radius) continue;
        float falloff = 1.0f - mathutil::Clamp01(dist / ex.radius);
        bool killed = z->TakeDamage(ex.damage * falloff);
        z->velocity = Vector2Add(z->velocity, mathutil::RadialImpulse(z->position, ex.position, kExplosionImpulse, dt));
        economy_.AddPoints(10, z->position);
        if (killed) AwardKill(*z, 100);
    }

    Vector2 diff = Vector2Subtract(player_.position, ex.position);
    float dist = Vector2Length(diff);
    if (dist < ex.radius) {
        float falloff = 1.0f - mathutil::Clamp01(dist / ex.radius);
        if (falloff > 0.5f && player_.ApplyContactDamage(ex.damage * falloff * 0.4f)) timeSinceLastHit_ = 0.0f;
        player_.velocity = Vector2Add(player_.velocity, mathutil::RadialImpulse(player_.position, ex.position, kExplosionImpulse, dt));
    }
}

void ZombiesMode::ResolveBulletHits() {
    auto& pool = projectiles_.Pool();
    for (auto& p : pool) {
        if (!p.active || p.kind != ProjectileKind::Bullet) continue;
        for (auto& z : zombies_) {
            if (!z->IsAlive()) continue;
            float dist = Vector2Distance(p.position, z->position);
            if (dist > p.radius + z->radius) continue;

            bool killed = z->TakeDamage(p.damage);
            if (p.slowDuration > 0.0f) z->ApplySlow(p.slowDuration);
            Vector2 diff = Vector2Subtract(z->position, p.position);
            if (Vector2LengthSqr(diff) > 0.0001f) {
                z->velocity = Vector2Add(z->velocity, Vector2Scale(Vector2Normalize(diff), 140.0f));
            }
            particles_.SpawnBurst(p.position, 6, Color{255, 220, 140, 255}, 40.0f, 160.0f, 0.1f, 0.25f);
            economy_.AddPoints(10, p.position);
            if (killed) AwardKill(*z, 100);
            p.active = false;
            break;
        }
    }
}

void ZombiesMode::UpdateZombies(float dt, Rectangle playArea, const Zone& zone) {
    for (auto& z : zombies_) {
        if (!z->IsAlive()) continue;
        z->UpdateAI(dt, player_.position, zombies_, projectiles_);

        // Soft steering away from nearby obstacles (no real pathfinding —
        // just enough to curve around furniture instead of stalling on it),
        // same technique LevelManager uses for Story Mode.
        for (const Obstacle& obs : zone.obstacles) {
            Vector2 obsCenter{zone.bounds.x + obs.center.x, zone.bounds.y + obs.center.y};
            Vector2 diff = Vector2Subtract(z->position, obsCenter);
            float d = Vector2Length(diff);
            float avoidRange = obs.radius + cfg::kObstacleAvoidRadius;
            if (d < avoidRange && d > 0.0001f) {
                float strength = (avoidRange - d) / avoidRange;
                z->velocity = Vector2Add(z->velocity, Vector2Scale(Vector2Scale(diff, 1.0f / d), strength * cfg::kObstacleAvoidForce));
            }
        }

        z->Update(dt, playArea);

        for (const Obstacle& obs : zone.obstacles) {
            Vector2 obsCenter{zone.bounds.x + obs.center.x, zone.bounds.y + obs.center.y};
            z->position = mathutil::ResolveCircleObstacle(z->position, z->radius, obsCenter, obs.radius);
        }

        float dist = Vector2Distance(z->position, player_.position);
        if (dist < z->radius + player_.radius) {
            float dmg = z->TryContactDamage();
            if (dmg > 0.0f && player_.ApplyContactDamage(dmg)) {
                timeSinceLastHit_ = 0.0f;
                Vector2 away = Vector2Subtract(player_.position, z->position);
                if (Vector2LengthSqr(away) > 0.0001f) {
                    player_.velocity = Vector2Add(player_.velocity, Vector2Scale(Vector2Normalize(away), 180.0f));
                }
                particles_.SpawnBurst(player_.position, 8, RED, 60.0f, 200.0f, 0.15f, 0.35f);
                audio_.Play(Sfx::PlayerHurt, 0.8f, 0.15f);
            }
        }

        // SodaBomber/BossBurrower-as-special detonate via RequestAoe; nothing
        // else in this mode's roster uses it, but any future addition that
        // does will resolve correctly here too.
        Enemy::AoeRequest aoe;
        if (z->ConsumeAoeRequest(aoe)) {
            particles_.SpawnBurst(aoe.origin, 40, Color{255, 140, 60, 255}, 120.0f, 480.0f, 0.3f, 0.7f, 3.0f, 7.0f);
            shake_.Trigger(0.4f, 14.0f);
            audio_.Play(Sfx::BossSlam);
            float pdist = Vector2Distance(player_.position, aoe.origin);
            if (pdist < aoe.radius) {
                float falloff = 1.0f - mathutil::Clamp01(pdist / aoe.radius);
                if (player_.ApplyContactDamage(aoe.damage * falloff)) {
                    timeSinceLastHit_ = 0.0f;
                    audio_.Play(Sfx::PlayerHurt, 0.9f, 0.1f);
                    if (aoe.slowDuration > 0.0f) player_.ApplySlow(aoe.slowDuration);
                }
                player_.velocity = Vector2Add(player_.velocity, mathutil::RadialImpulse(player_.position, aoe.origin, aoe.impulseStrength, dt));
            }
        }
    }

    for (auto it = zombies_.begin(); it != zombies_.end();) {
        if (!(*it)->IsAlive()) {
            it = zombies_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& [type, pos] : pendingSplits_) {
        zombies_.push_back(MakeEnemy(type, pos, /*elite=*/false, /*statMult=*/1.0f, /*isChild=*/true));
    }
    pendingSplits_.clear();
}

std::string ZombiesMode::CurrentPrompt() const {
    const Barrier* barrier = map_.NearbyUnclearedBarrier(player_.position, kStationInteractRange);
    if (barrier) return barrier->PromptText();

    for (const WallBuy& wb : wallBuys_) {
        if (wb.InRange(player_.position)) return wb.PromptText(equippedWeapon_);
    }

    return mysteryBox_.PromptText(player_.position);
}

void ZombiesMode::Update(float dt) {
    audio_.Update();

    if (state_ == RunState::GameOver) {
        if (IsKeyPressed(KEY_R)) Enter();
        return;
    }

    timeSinceLastHit_ += dt;
    if (timeSinceLastHit_ > kRegenDelay) {
        player_.Heal(kRegenRate * dt);
    }

    Camera2D camera = BuildCamera();
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

    player_.HandleInput(dt, mouseWorld);
    int zoneIdx = map_.ZoneIndexContaining(player_.position);
    Rectangle playArea = map_.PlayAreaFor(zoneIdx);
    player_.Update(dt, playArea);
    const Zone& currentZone = map_.Zones()[static_cast<size_t>(zoneIdx)];
    for (const Obstacle& obs : currentZone.obstacles) {
        Vector2 obsCenter{currentZone.bounds.x + obs.center.x, currentZone.bounds.y + obs.center.y};
        player_.position = mathutil::ResolveCircleObstacle(player_.position, player_.radius, obsCenter, obs.radius);
    }
    if (player_.ConsumeJustDashed()) audio_.Play(Sfx::Dash, 0.6f);

    HandleInteract();

    if (weaponCooldownTimer_ > 0.0f) weaponCooldownTimer_ -= dt;
    if (swingTimer_ > 0.0f) swingTimer_ -= dt;
    UpdateReload(dt);

    Vector2 aimDir = mathutil::FromAngle(player_.AimAngle());
    HandleAttack(player_.position, aimDir, IsMouseButtonDown(MOUSE_BUTTON_LEFT), IsMouseButtonPressed(MOUSE_BUTTON_LEFT), dt);

    for (const Explosion& ex : projectiles_.PopExplosions()) {
        ResolveExplosion(ex, dt);
    }

    projectiles_.Update(dt, playArea);
    ResolveBulletHits();

    UpdateZombies(dt, playArea, currentZone);
    director_.Update(dt, map_, zombies_);
    mysteryBox_.Update(dt);

    economy_.Update(dt);
    particles_.Update(dt);
    shake_.Update(dt);

    if (!player_.IsAlive()) {
        state_ = RunState::GameOver;
        audio_.Play(Sfx::GameOver);
    }
}

void ZombiesMode::Draw() {
    BeginDrawing();
    ClearBackground(BLACK);

    Camera2D camera = BuildCamera();
    BeginMode2D(camera);
    int zoneIdx = map_.ZoneIndexContaining(player_.position);
    DrawZone(map_.Zones()[static_cast<size_t>(zoneIdx)], zoneIdx);
    for (const Barrier& b : map_.Barriers()) DrawBarrier(b);
    for (const WallBuy& wb : wallBuys_) DrawWallBuy(wb, equippedWeapon_);
    mysteryBox_.Draw();

    for (const auto& z : zombies_) {
        if (!z->IsAlive()) continue;
        z->Draw();
        float frac = z->maxHealth > 0.0f ? z->health / z->maxHealth : 0.0f;
        float barW = z->radius * 2.2f;
        Vector2 barPos{z->position.x - barW * 0.5f, z->position.y - z->radius - 14.0f};
        DrawRectangle(static_cast<int>(barPos.x), static_cast<int>(barPos.y), static_cast<int>(barW), 5, Fade(DARKGRAY, 0.75f));
        DrawRectangle(static_cast<int>(barPos.x), static_cast<int>(barPos.y), static_cast<int>(barW * mathutil::Clamp01(frac)), 5,
                      Color{200, 50, 50, 255});
    }
    projectiles_.Draw();
    particles_.Draw();
    economy_.Draw();

    const ZombieWeaponStats& equippedStats = GetZombieWeaponStats(equippedWeapon_);
    if (equippedStats.fireMode == FireMode::Melee && swingTimer_ > 0.0f) {
        float halfArc = mathutil::DegToRadF(equippedStats.arcDeg) * 0.5f;
        float t = 1.0f - mathutil::Clamp01(swingTimer_ / kMeleeSwingVisualDuration);
        float bladeAngle = swingAngle_ - halfArc + 2.0f * halfArc * t;
        Vector2 tip = Vector2Add(player_.position, mathutil::FromAngle(bladeAngle, equippedStats.range));
        DrawLineEx(player_.position, tip, 5.0f, Color{140, 220, 90, static_cast<unsigned char>(255 * (1.0f - t))});
    }

    player_.Draw();
    EndMode2D();

    // --- HUD ---
    DrawHealthBar(Vector2{20, 20}, 260, 22, player_.health / player_.maxHealth, Color{60, 200, 90, 255});
    DrawText(TextFormat("HP %d/%d", static_cast<int>(player_.health), static_cast<int>(player_.maxHealth)), 28, 22, 16, RAYWHITE);

    std::string roundText = "ROUND " + ToRoman(director_.Round());
    int rw = MeasureText(roundText.c_str(), 24);
    DrawText(roundText.c_str(), cfg::kScreenWidth / 2 - rw / 2, 20, 24, RAYWHITE);
    if (director_.InDowntime()) {
        std::string next = TextFormat("Next round in %.0fs", director_.DowntimeRemaining());
        int nw = MeasureText(next.c_str(), 16);
        DrawText(next.c_str(), cfg::kScreenWidth / 2 - nw / 2, 48, 16, GOLD);
    }

    DrawText(TextFormat("CRUMBS: %d", economy_.Points()), cfg::kScreenWidth - 220, 20, 20, GOLD);

    DrawText(equippedStats.name, 20, cfg::kScreenHeight - 60, 18, WeaponColorFor(equippedWeapon_));
    if (equippedStats.magazineSize > 0) {
        if (reloading_) {
            DrawText("RELOADING...", 20, cfg::kScreenHeight - 38, 14, ORANGE);
        } else {
            DrawText(TextFormat("AMMO %d/%d", ammo_, equippedStats.magazineSize), 20, cfg::kScreenHeight - 38, 14, LIGHTGRAY);
        }
    }
    DrawText("[F2] Story Mode", 20, cfg::kScreenHeight - 20, 14, GRAY);

    std::string prompt = CurrentPrompt();
    if (!prompt.empty()) {
        int pw = MeasureText(prompt.c_str(), 20);
        DrawRectangle(cfg::kScreenWidth / 2 - pw / 2 - 10, cfg::kScreenHeight - 110, pw + 20, 30, Fade(BLACK, 0.65f));
        DrawText(prompt.c_str(), cfg::kScreenWidth / 2 - pw / 2, cfg::kScreenHeight - 104, 20, GOLD);
    }

    if (state_ == RunState::GameOver) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.7f));
        const char* msg = "YOU DIED";
        int w = MeasureText(msg, 60);
        DrawText(msg, cfg::kScreenWidth / 2 - w / 2, cfg::kScreenHeight / 2 - 60, 60, RED);
        std::string roundMsg = "Reached Round " + ToRoman(director_.Round());
        int w2 = MeasureText(roundMsg.c_str(), 24);
        DrawText(roundMsg.c_str(), cfg::kScreenWidth / 2 - w2 / 2, cfg::kScreenHeight / 2 + 10, 24, RAYWHITE);
        const char* hint = "Press R to try again, or [F2] for Story Mode";
        int w3 = MeasureText(hint, 18);
        DrawText(hint, cfg::kScreenWidth / 2 - w3 / 2, cfg::kScreenHeight / 2 + 46, 18, LIGHTGRAY);
    }

    EndDrawing();
}
