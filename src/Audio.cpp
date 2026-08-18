#include "Audio.hpp"
#include <cstdlib>
#include <functional>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Low-level synthesis helpers. Everything below runs once at startup to
// build raw PCM buffers; none of it runs per-frame.
// ---------------------------------------------------------------------------

float NoteHz(int semitoneFromA4) { return 440.0f * std::pow(2.0f, static_cast<float>(semitoneFromA4) / 12.0f); }

float SquareWave(float freqHz, float t) { return std::fmod(t * freqHz, 1.0f) < 0.5f ? 1.0f : -1.0f; }
float TriangleWave(float freqHz, float t) {
    float phase = std::fmod(t * freqHz, 1.0f);
    return phase < 0.5f ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
}
float SineWave(float freqHz, float t) { return std::sin(2.0f * PI * freqHz * t); }

// White noise, [-1, 1].
float Noise() { return static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f; }

Wave BuildWave(float durationSec, const std::function<float(float t)>& gen) {
    int frameCount = std::max(1, static_cast<int>(durationSec * cfg::kAudioSampleRate));
    Wave w{};
    w.frameCount = static_cast<unsigned int>(frameCount);
    w.sampleRate = cfg::kAudioSampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    auto* buf = static_cast<short*>(malloc(sizeof(short) * static_cast<size_t>(frameCount)));
    for (int i = 0; i < frameCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(cfg::kAudioSampleRate);
        float v = gen(t);
        v = std::max(-1.0f, std::min(1.0f, v));
        buf[i] = static_cast<short>(v * 30000.0f);
    }
    w.data = buf;
    return w;
}

Sound LoadGeneratedSound(float durationSec, const std::function<float(float t)>& gen) {
    Wave w = BuildWave(durationSec, gen);
    Sound s = LoadSoundFromWave(w);
    free(w.data);
    return s;
}

// ---------------------------------------------------------------------------
// Individual SFX generators. Each returns a short procedurally-synthesized
// clip; envelopes are hand-tuned exp/linear decays, no external assets.
// ---------------------------------------------------------------------------

Sound GenSwordSwing() {
    return LoadGeneratedSound(0.10f, [](float t) {
        float env = std::exp(-t * 35.0f);
        float sweep = SineWave(1200.0f - 5000.0f * t, t);
        return (Noise() * 0.5f + sweep * 0.5f) * env;
    });
}

Sound GenShieldBash() {
    return LoadGeneratedSound(0.09f, [](float t) {
        float env = std::exp(-t * 45.0f);
        return (SquareWave(180.0f, t) * 0.6f + Noise() * 0.4f) * env;
    });
}

Sound GenBlasterShot() {
    return LoadGeneratedSound(0.07f, [](float t) {
        float env = std::exp(-t * 50.0f);
        float freq = 1900.0f - 15000.0f * t;
        return SineWave(std::max(200.0f, freq), t) * env;
    });
}

Sound GenBombExplosion() {
    return LoadGeneratedSound(0.45f, [](float t) {
        float env = std::exp(-t * 6.0f);
        float thump = SineWave(55.0f, t) * std::exp(-t * 12.0f);
        return (Noise() * 0.7f * env) + thump * 0.6f;
    });
}

Sound GenHitLanded() {
    return LoadGeneratedSound(0.05f, [](float t) {
        float env = std::exp(-t * 70.0f);
        return SquareWave(240.0f, t) * env;
    });
}

Sound GenEnemyDeath() {
    return LoadGeneratedSound(0.2f, [](float t) {
        float env = std::exp(-t * 12.0f);
        float freq = 320.0f - 900.0f * t;
        return (SquareWave(std::max(60.0f, freq), t) * 0.55f + Noise() * 0.45f) * env;
    });
}

Sound GenDash() {
    float lp = 0.0f; // crude low-pass state, captured by reference below
    return LoadGeneratedSound(0.16f, [&lp](float t) {
        float env = std::sin(PI * std::min(1.0f, t / 0.16f)); // rise then fall
        lp = lp * 0.85f + Noise() * 0.15f; // low-pass filtered noise for a "whoosh" texture
        return lp * env;
    });
}

Sound GenPlayerHurt() {
    return LoadGeneratedSound(0.14f, [](float t) {
        float env = std::exp(-t * 18.0f);
        float freq = 260.0f - 160.0f * t;
        return (SquareWave(freq, t) * 0.6f + Noise() * 0.4f) * env;
    });
}

// Short envelope helper for a single note inside a chime/fanfare sequence.
float NoteEnvelope(float t, float start, float len) {
    if (t < start || t > start + len) return 0.0f;
    float local = t - start;
    float attack = std::min(1.0f, local / 0.015f);
    float release = std::min(1.0f, (len - local) / 0.05f);
    return std::min(attack, release);
}

Sound GenPickupHealth() {
    static const int notes[] = {-9, -5, -2}; // C5, E5, G5 relative to A4
    return LoadGeneratedSound(0.24f, [](float t) {
        float v = 0.0f;
        for (int i = 0; i < 3; ++i) {
            float start = i * 0.06f;
            v += SineWave(NoteHz(notes[i]), t) * NoteEnvelope(t, start, 0.09f);
        }
        return v * 0.6f;
    });
}

Sound GenPickupPowerUp() {
    static const int notes[] = {-9, -5, -2, 3}; // C5 E5 G5 C6
    return LoadGeneratedSound(0.4f, [](float t) {
        float v = 0.0f;
        for (int i = 0; i < 4; ++i) {
            float start = i * 0.06f;
            v += TriangleWave(NoteHz(notes[i]), t) * NoteEnvelope(t, start, 0.12f);
        }
        v += Noise() * 0.15f * std::exp(-t * 6.0f);
        return v * 0.55f;
    });
}

Sound GenGateUnlock() {
    static const int notes[] = {-2, 3}; // G4, C5
    return LoadGeneratedSound(0.22f, [](float t) {
        float v = 0.0f;
        for (int i = 0; i < 2; ++i) {
            float start = i * 0.08f;
            v += SquareWave(NoteHz(notes[i]), t) * 0.5f * NoteEnvelope(t, start, 0.12f);
        }
        return v;
    });
}

Sound GenBossSlam() {
    return LoadGeneratedSound(0.5f, [](float t) {
        float env = std::exp(-t * 5.0f);
        float thump = SineWave(42.0f, t) * std::exp(-t * 8.0f);
        return (Noise() * 0.6f * env) + thump * 0.75f;
    });
}

Sound GenBossPhase2() {
    return LoadGeneratedSound(0.55f, [](float t) {
        float env = std::exp(-t * 4.0f);
        float sweep = SineWave(140.0f + 260.0f * t, t);
        return (sweep * 0.6f + Noise() * 0.4f) * env;
    });
}

Sound GenLevelComplete() {
    static const int notes[] = {-9, -5, -2, 3}; // C5 E5 G5 C6
    return LoadGeneratedSound(0.65f, [](float t) {
        float v = 0.0f;
        for (int i = 0; i < 4; ++i) {
            float start = i * 0.13f;
            v += TriangleWave(NoteHz(notes[i]), t) * NoteEnvelope(t, start, 0.22f);
        }
        return v * 0.6f;
    });
}

Sound GenGameOver() {
    static const int notes[] = {-2, -5, -9}; // G4 E4 C4, descending
    return LoadGeneratedSound(0.75f, [](float t) {
        float v = 0.0f;
        for (int i = 0; i < 3; ++i) {
            float start = i * 0.2f;
            v += SquareWave(NoteHz(notes[i]), t) * 0.5f * NoteEnvelope(t, start, 0.32f);
        }
        return v;
    });
}

Sound GenVictory() {
    static const int notes[] = {-9, -5, -2, 3, 7, 3}; // C5 E5 G5 C6 E6 C6
    return LoadGeneratedSound(1.05f, [](float t) {
        float v = 0.0f;
        for (int i = 0; i < 6; ++i) {
            float start = i * 0.14f;
            v += TriangleWave(NoteHz(notes[i]), t) * NoteEnvelope(t, start, 0.24f);
        }
        return v * 0.55f;
    });
}

// ---------------------------------------------------------------------------
// Background music: a short looping 4-bar chiptune pattern (bassline +
// arpeggiated lead over an Am - F - C - G progression), synthesized once.
// ---------------------------------------------------------------------------

Sound GenMusicLoop() {
    constexpr float kBpm = 130.0f;
    constexpr float kBeat = 60.0f / kBpm;
    constexpr float kBarLen = kBeat * 4.0f;
    constexpr float kTotal = kBarLen * 4.0f;

    // Root note (bass, whole note per bar) and triad (lead arpeggio) per bar.
    static const int bassRoot[4] = {-12, -16, -9, -14};       // A3  F3  C4  G3
    static const int leadTriad[4][3] = {
        {-12, -9, -5},   // Am: A C E
        {-16, -12, -9},  // F:  F A C
        {-9, -5, -2},    // C:  C E G
        {-14, -10, -7},  // G:  G B D
    };

    return LoadGeneratedSound(kTotal, [=](float t) {
        int bar = std::min(3, static_cast<int>(t / kBarLen));
        float tBar = t - bar * kBarLen;

        // Bass: sustained triangle wave for the whole bar, soft attack/release.
        float bassEnv = std::min(1.0f, tBar / 0.04f) * std::min(1.0f, (kBarLen - tBar) / 0.06f);
        float bass = TriangleWave(NoteHz(bassRoot[bar] - 12), t) * bassEnv * 0.35f;

        // Lead: 8th-note arpeggio cycling through the bar's triad.
        constexpr float kEighth = kBeat * 0.5f;
        int step = static_cast<int>(tBar / kEighth);
        float tStep = tBar - step * kEighth;
        int note = leadTriad[bar][step % 3];
        float leadEnv = std::min(1.0f, tStep / 0.01f) * std::min(1.0f, (kEighth - tStep) / 0.03f);
        float lead = SquareWave(NoteHz(note + 12), t) * leadEnv * 0.18f;

        return bass + lead;
    });
}

}  // namespace

void AudioManager::Init() {
    if (initialized_) return;
    InitAudioDevice();

    sfx_[static_cast<size_t>(Sfx::SwordSwing)] = GenSwordSwing();
    sfx_[static_cast<size_t>(Sfx::ShieldBash)] = GenShieldBash();
    sfx_[static_cast<size_t>(Sfx::BlasterShot)] = GenBlasterShot();
    sfx_[static_cast<size_t>(Sfx::BombExplosion)] = GenBombExplosion();
    sfx_[static_cast<size_t>(Sfx::HitLanded)] = GenHitLanded();
    sfx_[static_cast<size_t>(Sfx::EnemyDeath)] = GenEnemyDeath();
    sfx_[static_cast<size_t>(Sfx::Dash)] = GenDash();
    sfx_[static_cast<size_t>(Sfx::PlayerHurt)] = GenPlayerHurt();
    sfx_[static_cast<size_t>(Sfx::PickupHealth)] = GenPickupHealth();
    sfx_[static_cast<size_t>(Sfx::PickupPowerUp)] = GenPickupPowerUp();
    sfx_[static_cast<size_t>(Sfx::GateUnlock)] = GenGateUnlock();
    sfx_[static_cast<size_t>(Sfx::BossSlam)] = GenBossSlam();
    sfx_[static_cast<size_t>(Sfx::BossPhase2)] = GenBossPhase2();
    sfx_[static_cast<size_t>(Sfx::LevelComplete)] = GenLevelComplete();
    sfx_[static_cast<size_t>(Sfx::GameOver)] = GenGameOver();
    sfx_[static_cast<size_t>(Sfx::Victory)] = GenVictory();

    musicLoop_ = GenMusicLoop();
    SetMusicVolume(cfg::kDefaultMusicVolume);
    sfxVolumeScale_ = cfg::kDefaultSfxVolume;

    PlaySound(musicLoop_);
    initialized_ = true;
}

void AudioManager::Shutdown() {
    if (!initialized_) return;
    for (auto& s : sfx_) UnloadSound(s);
    UnloadSound(musicLoop_);
    CloseAudioDevice();
    initialized_ = false;
}

void AudioManager::Update() {
    if (!initialized_) return;
    if (!IsSoundPlaying(musicLoop_)) PlaySound(musicLoop_);
}

void AudioManager::Play(Sfx sfx, float volume, float pitchVariance) {
    if (!initialized_) return;
    Sound& s = sfx_[static_cast<size_t>(sfx)];
    SetSoundVolume(s, std::max(0.0f, std::min(1.0f, volume)) * sfxVolumeScale_);
    if (pitchVariance > 0.0f) {
        SetSoundPitch(s, 1.0f + mathutil::RandomFloat(-pitchVariance, pitchVariance));
    } else {
        SetSoundPitch(s, 1.0f);
    }
    PlaySound(s);
}

void AudioManager::SetMusicVolume(float v) { SetSoundVolume(musicLoop_, std::max(0.0f, std::min(1.0f, v))); }
