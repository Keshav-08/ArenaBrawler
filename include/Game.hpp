#pragma once

#include "Audio.hpp"
#include "Common.hpp"
#include "LevelManager.hpp"
#include "ParticleSystem.hpp"
#include "Pickup.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include "Weapon.hpp"
#include <memory>
#include <vector>

// Top-level orchestrator: owns every subsystem, drives the update/render
// pipeline, and resolves cross-system interactions (bullet-vs-enemy
// collision, camera framing, HUD, debug overlay) that would otherwise create
// circular dependencies between the individual systems.
class Game {
public:
    Game();

    void Run(); // owns the main loop (window init/shutdown included)

private:
    void Update(float dt);
    void Draw();

    void HandleWeaponSwitch();
    void ResolveBulletHits();
    void ResolvePickupCollection();
    void RestartGame();
    void StartLevel(bool advancing); // advancing=false for a fresh game, true after LevelComplete
    void ApplyDifficulty(Difficulty d);
    Camera2D BuildCamera() const;
    void TriggerHitStop(float duration);

    Player player_;
    ProjectileManager projectiles_;
    ParticleSystem particles_;
    LevelManager levels_;
    PickupManager pickups_;
    ScreenShake shake_;
    ComboTracker combo_;
    PowerUpState powerUps_;
    AudioManager audio_;

    std::vector<std::unique_ptr<Weapon>> weapons_;
    int currentWeapon_ = 0;

    GameState state_ = GameState::LevelIntro;
    float stateTimer_ = cfg::kDifficultyPromptDuration;
    float hitStopTimer_ = 0.0f;
    bool debugMode_ = false;
    bool audioMuted_ = false;
    int score_ = 0;

    Difficulty difficulty_ = Difficulty::Normal;
    bool difficultyChosen_ = false;
};
