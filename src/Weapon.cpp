#include "Weapon.hpp"
#include "Enemy.hpp"

// ===========================================================================
// CelerySword
// ===========================================================================
CelerySword::CelerySword(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                          ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score)
    : Weapon("Celery Sword", projectiles, levels, particles, shake, player, combo, pickups, powerUps, audio, score) {}

void CelerySword::Update(float dt) {
    if (cooldownTimer_ > 0.0f) cooldownTimer_ -= dt;
    if (swingTimer_ > 0.0f) swingTimer_ -= dt;
}

void CelerySword::Attack(Vector2 origin, Vector2 dir) {
    if (!CanAttack()) return;
    cooldownTimer_ = cfg::kSwordCooldown;
    swingTimer_ = cfg::kSwordSwingDuration;
    swingAngle_ = mathutil::AngleOf(dir);
    audio_.Play(Sfx::SwordSwing, 0.7f, 0.2f);

    const float halfArc = mathutil::DegToRadF(cfg::kSwordArcDeg) * 0.5f;
    bool hitAnything = false;

    for (auto& e : levels_.GetEnemies()) {
        if (!e->IsAlive()) continue;
        Vector2 toEnemy = Vector2Subtract(e->position, origin);
        float dist = Vector2Length(toEnemy);
        if (dist > cfg::kSwordRange + e->radius) continue;
        if (dist < 0.0001f) continue;

        float angleToEnemy = mathutil::AngleOf(toEnemy);
        float diff = std::fabs(mathutil::AngleDiff(swingAngle_, angleToEnemy));
        if (diff > halfArc) continue;

        hitAnything = true;
        float dmg = cfg::kSwordDamage;
        bool juggled = Vector2Length(e->velocity) > cfg::kJuggleVelocityThreshold;
        if (juggled) dmg *= cfg::kJuggleDamageMult;
        dmg = ApplyDamageBuffs(*e, dmg, /*isMelee=*/true);

        bool killed = e->TakeDamage(dmg);
        e->MarkForBonus(cfg::kMarkedDuration);
        Vector2 pushDir = Vector2Scale(toEnemy, 1.0f / dist);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(pushDir, cfg::kSwordKnockback));
        particles_.SpawnBurst(e->position, 10, juggled ? Color{255, 230, 120, 255} : Color{240, 240, 240, 255},
                               80.0f, 260.0f, 0.15f, 0.35f);

        if (killed) {
            particles_.SpawnBurst(e->position, 16, Color{200, 40, 40, 255}, 60.0f, 260.0f, 0.25f, 0.5f, 3.0f, 6.0f);
            OnKill(e->position, 25);
        }
    }

    if (hitAnything) {
        shake_.Trigger(0.18f, 5.5f);
        audio_.Play(Sfx::HitLanded, 0.7f, 0.2f);
    }
}

void CelerySword::Draw(Vector2 origin) const {
    if (swingTimer_ <= 0.0f) return;
    float t = 1.0f - mathutil::Clamp01(swingTimer_ / cfg::kSwordSwingDuration);
    float halfArc = mathutil::DegToRadF(cfg::kSwordArcDeg) * 0.5f;
    // Sweep the blade tip across the arc as the swing progresses.
    float bladeAngle = swingAngle_ - halfArc + (2.0f * halfArc * t);
    Vector2 tip = Vector2Add(origin, mathutil::FromAngle(bladeAngle, cfg::kSwordRange));
    DrawLineEx(origin, tip, 5.0f, Color{140, 220, 90, static_cast<unsigned char>(255 * (1.0f - t))});
    DrawCircleSector(origin, cfg::kSwordRange, mathutil::RadToDegF(swingAngle_ - halfArc),
                      mathutil::RadToDegF(bladeAngle), 16, Fade(GREEN, 0.12f));
}

void CelerySword::DrawUI(Vector2 screenPos) const {
    DrawText("CELERY SWORD", static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 18, RAYWHITE);
    float frac = mathutil::Clamp01(1.0f - cooldownTimer_ / cfg::kSwordCooldown);
    Rectangle bar{screenPos.x, screenPos.y + 22, 160, 10};
    DrawRectangleRec(bar, Fade(DARKGRAY, 0.6f));
    DrawRectangle(static_cast<int>(bar.x), static_cast<int>(bar.y), static_cast<int>(bar.width * frac),
                  static_cast<int>(bar.height), LIME);
    DrawRectangleLinesEx(bar, 1.0f, RAYWHITE);
}

// ===========================================================================
// ChurroBlaster
// ===========================================================================
ChurroBlaster::ChurroBlaster(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                              ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score)
    : Weapon("Churro Blaster", projectiles, levels, particles, shake, player, combo, pickups, powerUps, audio, score) {}

void ChurroBlaster::Update(float dt) {
    if (cooldownTimer_ > 0.0f) cooldownTimer_ -= dt;
    if (muzzleFlashTimer_ > 0.0f) muzzleFlashTimer_ -= dt;

    if (powerUps_.RapidFire() && ammo_ <= 0) {
        ammo_ = cfg::kBlasterMagazine; // Rapid Fire also means unlimited ammo: skip the reload wait
        reloadTimer_ = 0.0f;
    } else if (ammo_ <= 0) {
        reloadTimer_ -= dt;
        if (reloadTimer_ <= 0.0f) {
            ammo_ = cfg::kBlasterMagazine;
        }
    }
}

void ChurroBlaster::Attack(Vector2 origin, Vector2 dir) {
    if (!CanAttack()) return;
    cooldownTimer_ = powerUps_.RapidFire() ? cfg::kRapidFireCooldown : 1.0f / cfg::kBlasterFireRate;

    float baseAngle = mathutil::AngleOf(dir);
    float spread = mathutil::DegToRadF(mathutil::RandomFloat(-cfg::kBlasterSpreadDeg, cfg::kBlasterSpreadDeg));
    float angle = baseAngle + spread;
    lastAngle_ = angle;

    Vector2 vel = mathutil::FromAngle(angle, cfg::kBlasterBulletSpeed);
    Vector2 spawnPos = Vector2Add(origin, mathutil::FromAngle(angle, cfg::kPlayerRadius + 4.0f));
    projectiles_.SpawnBullet(spawnPos, vel, cfg::kBlasterBulletDamage, cfg::kBlasterBulletRadius, cfg::kBlasterBulletLife);

    particles_.SpawnMuzzleFlash(spawnPos, angle, Color{255, 200, 120, 255});
    muzzleFlashTimer_ = 0.06f;
    audio_.Play(Sfx::BlasterShot, 0.5f, 0.15f);

    if (!powerUps_.RapidFire()) {
        ammo_--;
        if (ammo_ <= 0) {
            reloadTimer_ = cfg::kBlasterReloadTime;
        }
    }
}

void ChurroBlaster::Draw(Vector2 origin) const {
    if (muzzleFlashTimer_ > 0.0f) {
        Vector2 tip = Vector2Add(origin, mathutil::FromAngle(lastAngle_, cfg::kPlayerRadius + 10.0f));
        DrawCircleV(tip, 6.0f, Fade(YELLOW, 0.8f));
    }
}

void ChurroBlaster::DrawUI(Vector2 screenPos) const {
    DrawText("CHURRO BLASTER", static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 18, RAYWHITE);
    if (ammo_ <= 0) {
        float frac = mathutil::Clamp01(1.0f - reloadTimer_ / cfg::kBlasterReloadTime);
        DrawText("RELOADING", static_cast<int>(screenPos.x), static_cast<int>(screenPos.y + 22), 14, ORANGE);
        Rectangle bar{screenPos.x, screenPos.y + 40, 160, 8};
        DrawRectangleRec(bar, Fade(DARKGRAY, 0.6f));
        DrawRectangle(static_cast<int>(bar.x), static_cast<int>(bar.y), static_cast<int>(bar.width * frac),
                      static_cast<int>(bar.height), ORANGE);
        DrawRectangleLinesEx(bar, 1.0f, RAYWHITE);
    } else {
        for (int i = 0; i < cfg::kBlasterMagazine; ++i) {
            Color c = i < ammo_ ? YELLOW : Fade(DARKGRAY, 0.6f);
            DrawRectangle(static_cast<int>(screenPos.x) + i * 11, static_cast<int>(screenPos.y) + 22, 8, 14, c);
        }
    }
}

// ===========================================================================
// BurritoBomb
// ===========================================================================
BurritoBomb::BurritoBomb(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                          ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score)
    : Weapon("Burrito Bomb", projectiles, levels, particles, shake, player, combo, pickups, powerUps, audio, score) {}

void BurritoBomb::Update(float dt) {
    if (cooldownTimer_ > 0.0f) cooldownTimer_ -= dt;

    for (const Explosion& ex : projectiles_.PopExplosions()) {
        particles_.SpawnBurst(ex.position, 40, Color{255, 170, 60, 255}, 120.0f, 480.0f, 0.3f, 0.7f, 3.0f, 7.0f);
        shake_.Trigger(0.35f, 12.0f);
        audio_.Play(Sfx::BombExplosion, 0.85f, 0.1f);

        for (auto& e : levels_.GetEnemies()) {
            if (!e->IsAlive()) continue;
            Vector2 diff = Vector2Subtract(e->position, ex.position);
            float dist = Vector2Length(diff);
            if (dist > ex.radius) continue;

            float falloff = 1.0f - mathutil::Clamp01(dist / ex.radius);
            float dmg = ApplyDamageBuffs(*e, ex.damage * falloff, /*isMelee=*/false);
            bool killed = e->TakeDamage(dmg);
            e->velocity = Vector2Add(e->velocity, mathutil::RadialImpulse(e->position, ex.position, cfg::kBombImpulseStrength, dt));
            if (killed) {
                particles_.SpawnBurst(e->position, 14, Color{200, 40, 40, 255}, 60.0f, 260.0f, 0.25f, 0.5f);
                OnKill(e->position, 25);
            }
        }

        // The blast also shoves the player around (and can hurt them up close).
        Vector2 diff = Vector2Subtract(player_.position, ex.position);
        float dist = Vector2Length(diff);
        if (dist < ex.radius) {
            float falloff = 1.0f - mathutil::Clamp01(dist / ex.radius);
            if (falloff > 0.5f) player_.ApplyContactDamage(ex.damage * falloff * 0.4f);
            player_.velocity = Vector2Add(player_.velocity, mathutil::RadialImpulse(player_.position, ex.position, cfg::kBombImpulseStrength, dt));
        }
    }
}

void BurritoBomb::Attack(Vector2 origin, Vector2 dir) {
    if (!CanAttack()) return;
    cooldownTimer_ = cfg::kBombCooldown;
    Vector2 vel = Vector2Scale(dir, cfg::kBombThrowSpeed);
    projectiles_.SpawnBomb(origin, vel, cfg::kBombDamage, cfg::kBombBlastRadius, cfg::kBombFuse);
}

void BurritoBomb::Draw(Vector2 origin) const { (void)origin; }

void BurritoBomb::DrawUI(Vector2 screenPos) const {
    DrawText("BURRITO BOMB", static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 18, RAYWHITE);
    float frac = mathutil::Clamp01(1.0f - cooldownTimer_ / cfg::kBombCooldown);
    Rectangle bar{screenPos.x, screenPos.y + 22, 160, 10};
    DrawRectangleRec(bar, Fade(DARKGRAY, 0.6f));
    DrawRectangle(static_cast<int>(bar.x), static_cast<int>(bar.y), static_cast<int>(bar.width * frac),
                  static_cast<int>(bar.height), ORANGE);
    DrawRectangleLinesEx(bar, 1.0f, RAYWHITE);
}

// ===========================================================================
// NachoShield
// ===========================================================================
NachoShield::NachoShield(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                          ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score)
    : Weapon("Nacho Shield", projectiles, levels, particles, shake, player, combo, pickups, powerUps, audio, score) {}

void NachoShield::Update(float dt) {
    bool wasBlocking = blocking_;
    if (wantsBlock_ && guard_ > 0.0f) {
        blocking_ = true;
        guard_ -= cfg::kShieldDrainPerSec * dt;
        if (guard_ <= 0.0f) { guard_ = 0.0f; blocking_ = false; }
    } else {
        blocking_ = false;
    }
    if (wasBlocking && !blocking_) regenDelayTimer_ = cfg::kShieldRegenDelay;

    if (!blocking_) {
        if (regenDelayTimer_ > 0.0f) {
            regenDelayTimer_ -= dt;
        } else {
            guard_ = std::min(cfg::kShieldGuardMax, guard_ + cfg::kShieldRegenPerSec * dt);
        }
    }

    if (bashCooldownTimer_ > 0.0f) bashCooldownTimer_ -= dt;
    if (bashSwingTimer_ > 0.0f) bashSwingTimer_ -= dt;
}

void NachoShield::Attack(Vector2 origin, Vector2 dir) {
    if (!CanAttack()) return;
    bashCooldownTimer_ = cfg::kShieldBashCooldown;
    bashSwingTimer_ = 0.14f;
    bashAngle_ = mathutil::AngleOf(dir);
    audio_.Play(Sfx::ShieldBash, 0.7f, 0.15f);

    const float halfArc = mathutil::DegToRadF(cfg::kShieldBashArcDeg) * 0.5f;
    bool hitAnything = false;

    for (auto& e : levels_.GetEnemies()) {
        if (!e->IsAlive()) continue;
        Vector2 toEnemy = Vector2Subtract(e->position, origin);
        float dist = Vector2Length(toEnemy);
        if (dist > cfg::kShieldBashRange + e->radius) continue;
        if (dist < 0.0001f) continue;

        float angleToEnemy = mathutil::AngleOf(toEnemy);
        float diff = std::fabs(mathutil::AngleDiff(bashAngle_, angleToEnemy));
        if (diff > halfArc) continue;

        hitAnything = true;
        float dmg = cfg::kShieldBashDamage;
        if (Vector2Length(e->velocity) > cfg::kJuggleVelocityThreshold) dmg *= cfg::kJuggleDamageMult;
        dmg = ApplyDamageBuffs(*e, dmg, /*isMelee=*/true);

        bool killed = e->TakeDamage(dmg);
        Vector2 pushDir = Vector2Scale(toEnemy, 1.0f / dist);
        e->velocity = Vector2Add(e->velocity, Vector2Scale(pushDir, cfg::kShieldBashKnockback));
        particles_.SpawnBurst(e->position, 8, Color{230, 200, 90, 255}, 100.0f, 300.0f, 0.15f, 0.3f);

        if (killed) {
            particles_.SpawnBurst(e->position, 14, Color{200, 40, 40, 255}, 60.0f, 260.0f, 0.25f, 0.5f);
            OnKill(e->position, 20);
        }
    }

    if (hitAnything) shake_.Trigger(0.15f, 6.0f);
}

void NachoShield::Draw(Vector2 origin) const {
    if (blocking_) {
        float facing = mathutil::RadToDegF(player_.AimAngle());
        DrawCircleSector(origin, 40.0f, facing - 55.0f, facing + 55.0f, 12, Fade(SKYBLUE, 0.35f));
        DrawRing(origin, 36.0f, 40.0f, facing - 55.0f, facing + 55.0f, 12, Fade(RAYWHITE, 0.8f));
    }
    if (bashSwingTimer_ > 0.0f) {
        float t = 1.0f - mathutil::Clamp01(bashSwingTimer_ / 0.14f);
        Vector2 tip = Vector2Add(origin, mathutil::FromAngle(bashAngle_, cfg::kShieldBashRange));
        DrawLineEx(origin, tip, 6.0f, Color{230, 200, 90, static_cast<unsigned char>(255 * (1.0f - t))});
    }
}

void NachoShield::DrawUI(Vector2 screenPos) const {
    DrawText("NACHO SHIELD", static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 18, RAYWHITE);
    float frac = mathutil::Clamp01(guard_ / cfg::kShieldGuardMax);
    Rectangle bar{screenPos.x, screenPos.y + 22, 160, 10};
    DrawRectangleRec(bar, Fade(DARKGRAY, 0.6f));
    Color guardColor = blocking_ ? SKYBLUE : (frac < 0.3f ? RED : SKYBLUE);
    DrawRectangle(static_cast<int>(bar.x), static_cast<int>(bar.y), static_cast<int>(bar.width * frac),
                  static_cast<int>(bar.height), guardColor);
    DrawRectangleLinesEx(bar, 1.0f, RAYWHITE);
    DrawText("[hold RMB] block  [LMB] bash", static_cast<int>(screenPos.x), static_cast<int>(screenPos.y + 36), 12, LIGHTGRAY);
}
