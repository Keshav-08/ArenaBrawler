#pragma once

#include "Audio.hpp"
#include "Common.hpp"
#include "LevelManager.hpp"
#include "ParticleSystem.hpp"
#include "Pickup.hpp"
#include "Projectile.hpp"
#include <string>

// Polymorphic weapon base. Concrete weapons are constructed with references
// to the shared systems they need (projectile pool for ranged/thrown attacks,
// the level manager for melee hit resolution, particle system for FX, the
// shared screen-shake accumulator, the combo tracker so kills scale score
// with the current streak, the pickup pool for on-kill drops, the active
// power-up buffs, and the running score) so weapons can resolve their own
// hits without Game knowing weapon-specific details.
class Weapon {
public:
    Weapon(std::string name, ProjectileManager& projectiles, LevelManager& levels,
           ParticleSystem& particles, ScreenShake& shake, Player& player, ComboTracker& combo,
           PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score)
        : name_(std::move(name)), projectiles_(projectiles), levels_(levels),
          particles_(particles), shake_(shake), player_(player), combo_(combo),
          pickups_(pickups), powerUps_(powerUps), audio_(audio), score_(score) {}

    virtual ~Weapon() = default;

    virtual void Update(float dt) = 0;
    virtual void Attack(Vector2 origin, Vector2 dir) = 0;
    virtual void Draw(Vector2 origin) const = 0;
    virtual void DrawUI(Vector2 screenPos) const = 0;

    virtual bool CanAttack() const = 0;

    // Held-block weapons (Nacho Shield) override these; every other weapon
    // keeps the no-op defaults.
    virtual void SetBlocking(bool) {}
    virtual bool IsBlocking() const { return false; }

    // Max Ammo power-up: refill ammo and zero this weapon's cooldown(s) so
    // it's instantly ready again. Default no-op; overridden per weapon.
    virtual void RefillAndResetCooldown() {}

    const std::string& Name() const { return name_; }

protected:
    // Awards `baseScore` (scaled by streak/difficulty/Double Points), and
    // rolls a chance to drop a pickup at `deathPos` — the common "an enemy
    // just died" bookkeeping shared by every weapon's kill branch.
    void OnKill(Vector2 deathPos, int baseScore) {
        score_ += combo_.RegisterKill(baseScore);
        pickups_.RollAndSpawnDrop(deathPos);
        audio_.Play(Sfx::EnemyDeath, 0.8f, 0.15f);
    }

    // Insta-Kill (any weapon) / Berserk (melee only) check: returns lethal
    // damage if Insta-Kill is active, otherwise `baseDamage` doubled while
    // Berserk is active and `isMelee` is true.
    float ApplyDamageBuffs(const Enemy& target, float baseDamage, bool isMelee) const {
        if (powerUps_.InstaKill()) return target.health + 1.0f;
        if (isMelee && powerUps_.Berserk()) return baseDamage * cfg::kBerserkDamageMult;
        return baseDamage;
    }

    std::string name_;
    ProjectileManager& projectiles_;
    LevelManager& levels_;
    ParticleSystem& particles_;
    ScreenShake& shake_;
    Player& player_;
    ComboTracker& combo_;
    PickupManager& pickups_;
    PowerUpState& powerUps_;
    AudioManager& audio_;
    int& score_;
};

// --- Celery Sword: melee swing, swept circular-arc hit detection ----------
class CelerySword : public Weapon {
public:
    CelerySword(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score);

    void Update(float dt) override;
    void Attack(Vector2 origin, Vector2 dir) override;
    void Draw(Vector2 origin) const override;
    void DrawUI(Vector2 screenPos) const override;
    bool CanAttack() const override { return cooldownTimer_ <= 0.0f; }
    void RefillAndResetCooldown() override { cooldownTimer_ = 0.0f; }

private:
    float cooldownTimer_ = 0.0f;
    float swingTimer_ = 0.0f;
    float swingAngle_ = 0.0f;
};

// --- Churro Blaster: ranged, pooled bullets, spread & fire rate -----------
class ChurroBlaster : public Weapon {
public:
    ChurroBlaster(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                  ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score);

    void Update(float dt) override;
    void Attack(Vector2 origin, Vector2 dir) override;
    void Draw(Vector2 origin) const override;
    void DrawUI(Vector2 screenPos) const override;
    bool CanAttack() const override { return cooldownTimer_ <= 0.0f && ammo_ > 0; }
    void RefillAndResetCooldown() override { cooldownTimer_ = 0.0f; reloadTimer_ = 0.0f; ammo_ = cfg::kBlasterMagazine; }

private:
    float cooldownTimer_ = 0.0f;
    float reloadTimer_ = 0.0f;
    int ammo_ = cfg::kBlasterMagazine;
    float muzzleFlashTimer_ = 0.0f;
    float lastAngle_ = 0.0f;
};

// --- Burrito Bomb: lobbed AoE with a fuse, inverse-square impulse ---------
class BurritoBomb : public Weapon {
public:
    BurritoBomb(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score);

    // Ticks the throw cooldown AND resolves any bomb detonations that
    // occurred this frame (radial damage + inverse-square impulse), so this
    // must run every frame regardless of which weapon is currently equipped.
    void Update(float dt) override;
    void Attack(Vector2 origin, Vector2 dir) override;
    void Draw(Vector2 origin) const override;
    void DrawUI(Vector2 screenPos) const override;
    bool CanAttack() const override { return cooldownTimer_ <= 0.0f; }
    void RefillAndResetCooldown() override { cooldownTimer_ = 0.0f; }

private:
    float cooldownTimer_ = 0.0f;
};

// --- Nacho Shield: hold to block, click to bash ----------------------------
class NachoShield : public Weapon {
public:
    NachoShield(ProjectileManager& projectiles, LevelManager& levels, ParticleSystem& particles,
                ScreenShake& shake, Player& player, ComboTracker& combo, PickupManager& pickups, PowerUpState& powerUps, AudioManager& audio, int& score);

    void Update(float dt) override;
    void Attack(Vector2 origin, Vector2 dir) override; // shield bash
    void Draw(Vector2 origin) const override;
    void DrawUI(Vector2 screenPos) const override;
    bool CanAttack() const override { return bashCooldownTimer_ <= 0.0f; }
    void RefillAndResetCooldown() override { bashCooldownTimer_ = 0.0f; guard_ = cfg::kShieldGuardMax; }

    void SetBlocking(bool held) override { wantsBlock_ = held; }
    bool IsBlocking() const override { return blocking_; }

private:
    float guard_ = cfg::kShieldGuardMax;
    bool wantsBlock_ = false; // input state, set every frame by Game
    bool blocking_ = false;   // actual state after guard-meter gating
    float regenDelayTimer_ = 0.0f;
    float bashCooldownTimer_ = 0.0f;
    float bashSwingTimer_ = 0.0f;
    float bashAngle_ = 0.0f;
};
