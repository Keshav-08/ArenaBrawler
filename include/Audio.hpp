#pragma once

#include "Common.hpp"
#include <array>

// Every one-shot sound effect the game can trigger. All are synthesized
// procedurally at startup (see Audio.cpp) — no external asset files.
enum class Sfx {
    SwordSwing,
    ShieldBash,
    BlasterShot,
    BombExplosion,
    HitLanded,
    EnemyDeath,
    Dash,
    PlayerHurt,
    PickupHealth,
    PickupPowerUp,
    GateUnlock,
    BossSlam,
    BossPhase2,
    LevelComplete,
    GameOver,
    Victory,
    Count
};

// Owns the audio device, every procedurally-generated SFX clip, and a
// looping procedural background track. Game calls Init once at startup,
// Update every frame (to keep the music loop going), and Play(...) at each
// gameplay event; Shutdown once before the window closes.
class AudioManager {
public:
    void Init();
    void Shutdown();
    void Update(); // restarts the music loop when it finishes playing

    void Play(Sfx sfx, float volume = 1.0f, float pitchVariance = 0.0f);

    void SetMusicVolume(float v);
    void SetSfxVolume(float v) { sfxVolumeScale_ = v; }
    float SfxVolumeScale() const { return sfxVolumeScale_; }

private:
    std::array<Sound, static_cast<size_t>(Sfx::Count)> sfx_{};
    Sound musicLoop_{};
    float sfxVolumeScale_ = 1.0f;
    bool initialized_ = false;
};
