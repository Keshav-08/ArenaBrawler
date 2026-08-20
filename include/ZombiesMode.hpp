#pragma once

#include "Audio.hpp"
#include "Common.hpp"
#include "Economy.hpp"
#include "Enemy.hpp"
#include "GameMode.hpp"
#include "MapGraph.hpp"
#include "ParticleSystem.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include "Station.hpp"
#include "ZombieWeapon.hpp"
#include "ZombiesDirector.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Standalone "Infinite Zombies Survival" mode: its own map (MapGraph), its
// own point economy (Economy, "Crumbs"), its own weapon system
// (ZombieWeaponKind/FireMode — hand-rolled combat, NOT StoryMode's Weapon
// hierarchy, which is coupled to LevelManager), and an infinite
// ZombiesDirector round loop. Deliberately shares nothing with StoryMode
// except the systems that were already fully generic (Player,
// Enemy/GlazedChaser/etc., ProjectileManager, ParticleSystem, ScreenShake,
// AudioManager).
class ZombiesMode : public IGameMode {
public:
    explicit ZombiesMode(AudioManager& audio);

    void Enter() override; // full reset: fresh map, player, round 1
    void Update(float dt) override;
    void Draw() override;
    void Exit() override {}

private:
    void HandleInteract();
    void HandleAttack(Vector2 origin, Vector2 aimDir, bool heldNow, bool pressedNow, float dt);

    // One resolver per FireMode; HandleAttack dispatches into these instead
    // of switching on ZombieWeaponKind, so most of the 30 guns need zero
    // new code — just a stats row (see ZombieWeapon.hpp).
    void FireMelee(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats);
    void FireBulletShot(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats); // SemiAuto/FullAuto/Freeze
    void FireBurst(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats);
    void FireShotgun(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats);
    void FireExplosive(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats);
    void FireContinuousCone(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats, float dt);
    void FireLaser(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats, float dt);
    void FireChain(Vector2 origin, Vector2 aimDir, const ZombieWeaponStats& stats);
    void UpdateBurst(float dt); // advances an in-progress burst started by FireBurst
    void StartReload(const ZombieWeaponStats& stats);
    void UpdateReload(float dt);

    void ResolveExplosion(const Explosion& ex, float dt);
    void ResolveBulletHits();
    void UpdateZombies(float dt, Rectangle playArea, const Zone& zone);
    void AwardKill(Enemy& zombie, int basePoints); // handles points/particles/audio + PickleSplitter split queuing
    Camera2D BuildCamera() const;
    std::string CurrentPrompt() const;

    AudioManager& audio_;

    Player player_;
    ProjectileManager projectiles_;
    ParticleSystem particles_;
    ScreenShake shake_;
    MapGraph map_;
    Economy economy_;
    ZombiesDirector director_;
    std::vector<WallBuy> wallBuys_;
    MysteryBox mysteryBox_;
    std::vector<std::unique_ptr<Enemy>> zombies_;
    std::vector<std::pair<EnemyType, Vector2>> pendingSplits_; // deferred PickleSplitter children

    ZombieWeaponKind equippedWeapon_ = ZombieWeaponKind::Sword;
    int ammo_ = 0;
    float weaponCooldownTimer_ = 0.0f;
    float swingTimer_ = 0.0f;
    float swingAngle_ = 0.0f;

    // Auto-reload (mirrors Story Mode's Churro Blaster): empty magazine
    // starts a timer that refills it, so running dry doesn't require a trip
    // back to a wall-buy — that just becomes a way to skip the wait.
    bool reloading_ = false;
    float reloadTimer_ = 0.0f;

    // Burst-fire in-progress state (Burst FireMode only).
    bool burstActive_ = false;
    int burstShotsRemaining_ = 0;
    float burstTimer_ = 0.0f;
    Vector2 burstOrigin_{};
    Vector2 burstAimDir_{};

    float timeSinceLastHit_ = 0.0f; // drives passive HP regen (no health pickups in this mode)

    enum class RunState { Playing, GameOver };
    RunState state_ = RunState::Playing;
};
