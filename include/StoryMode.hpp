#pragma once

#include "Audio.hpp"
#include "Common.hpp"
#include "GameMode.hpp"
#include "LevelManager.hpp"
#include "ParticleSystem.hpp"
#include "Pickup.hpp"
#include "Player.hpp"
#include "Projectile.hpp"
#include "Weapon.hpp"
#include <memory>
#include <vector>

// The original room/level/boss arcade campaign, unchanged in behavior from
// when this lived directly in Game — extracted so Game can also host
// ZombiesMode behind the same IGameMode interface. Owns every system it
// needs except the audio device itself (shared across modes; see Game).
class StoryMode : public IGameMode {
public:
    explicit StoryMode(AudioManager& audio);

    void Enter() override;   // full reset + starts level 1 (see RestartGame)
    void Update(float dt) override;
    void Draw() override;
    void Exit() override {}  // no-op: re-Enter() always resets fully

private:
    void HandleWeaponSwitch();
    void ResolveBulletHits();
    void ResolvePickupCollection();
    void RestartGame();
    void StartLevel(bool advancing); // advancing=false for a fresh game, true after LevelComplete
    void ApplyDifficulty(Difficulty d);
    Camera2D BuildCamera() const;
    void TriggerHitStop(float duration);

    AudioManager& audio_;

    Player player_;
    ProjectileManager projectiles_;
    ParticleSystem particles_;
    LevelManager levels_;
    PickupManager pickups_;
    ScreenShake shake_;
    ComboTracker combo_;
    PowerUpState powerUps_;

    // Loadout: slot 0 (Celery Sword) is always owned; slot2Weapon_ is the
    // index into weapons_ of whichever ranged/utility weapon was most
    // recently looted (-1 = nothing found yet). All 4 Weapon objects are
    // still constructed upfront so their per-frame ticking (e.g. a thrown
    // bomb detonating after switching away) keeps working unchanged.
    std::vector<std::unique_ptr<Weapon>> weapons_;
    int currentWeapon_ = 0;
    int slot2Weapon_ = -1;

    GameState state_ = GameState::LevelIntro;
    float stateTimer_ = cfg::kDifficultyPromptDuration;
    float hitStopTimer_ = 0.0f;
    bool debugMode_ = false;
    bool audioMuted_ = false;
    int score_ = 0;

    Difficulty difficulty_ = Difficulty::Normal;
    bool difficultyChosen_ = false;
};
