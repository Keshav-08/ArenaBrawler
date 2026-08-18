#pragma once

#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

// ---------------------------------------------------------------------------
// Screen / Arena constants
// ---------------------------------------------------------------------------
namespace cfg {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr const char* kWindowTitle = "Arena Brawler";

// ---------------------------------------------------------------------------
// Player tuning
// ---------------------------------------------------------------------------
constexpr float kPlayerRadius = 16.0f;
constexpr float kPlayerMaxHealth = 100.0f;
constexpr float kPlayerAcceleration = 2600.0f;   // px/s^2
constexpr float kPlayerFriction = 10.0f;         // damping factor (per second)
constexpr float kPlayerMaxSpeed = 320.0f;        // px/s

constexpr float kDashSpeed = 950.0f;
constexpr float kDashDuration = 0.16f;
constexpr float kDashCooldown = 0.85f;
constexpr float kDashIFrames = 0.20f;

constexpr float kContactIFrames = 0.5f;          // grace period after taking contact dmg

// ---------------------------------------------------------------------------
// Enemy tuning
// ---------------------------------------------------------------------------
constexpr float kGlazedChaserRadius = 12.0f;
constexpr float kGlazedChaserHealth = 22.0f;
constexpr float kGlazedChaserSpeed = 210.0f;
constexpr float kGlazedChaserDamage = 8.0f;
constexpr float kGlazedChaserSeparationRadius = 34.0f;
constexpr float kGlazedChaserSeparationForce = 420.0f;

constexpr float kTwistChargerRadius = 20.0f;
constexpr float kTwistChargerHealth = 60.0f;
constexpr float kTwistChargerSpeed = 90.0f;
constexpr float kTwistChargerDamage = 18.0f;
constexpr float kTwistChargerTelegraph = 0.65f;
constexpr float kTwistChargerChargeSpeed = 780.0f;
constexpr float kTwistChargerChargeDuration = 0.45f;
constexpr float kTwistChargerRecover = 1.1f;
constexpr float kTwistChargerTriggerRange = 260.0f;

constexpr float kCheddarShooterRadius = 14.0f;
constexpr float kCheddarShooterHealth = 26.0f;
constexpr float kCheddarShooterSpeed = 140.0f;
constexpr float kCheddarShooterDamage = 6.0f;
constexpr float kCheddarShooterPreferredRange = 320.0f;  // tries to hover at this distance
constexpr float kCheddarShooterRangeSlop = 40.0f;
constexpr float kCheddarShooterFireInterval = 1.6f;
constexpr float kCheddarShooterBulletSpeed = 360.0f;
constexpr float kCheddarShooterBulletDamage = 8.0f;
constexpr float kCheddarShooterBulletRadius = 5.0f;
constexpr float kCheddarShooterBulletLife = 2.2f;

constexpr float kEnemyContactCooldown = 0.75f; // per-enemy cooldown between dealing contact dmg

// Elite variants: stat multipliers applied on top of a minion's base stats.
constexpr float kEliteHealthMult = 2.2f;
constexpr float kEliteSpeedMult = 1.15f;
constexpr float kEliteDamageMult = 1.5f;
constexpr float kEliteRadiusMult = 1.25f;

// Weapon-synergy status effects (see Enemy::markedTimer_).
constexpr float kMarkedDuration = 2.0f;       // window after a sword hit where blaster bonus applies
constexpr float kMarkedDamageMult = 1.5f;
constexpr float kJuggleVelocityThreshold = 260.0f; // enemy speed above which it counts as "airborne"
constexpr float kJuggleDamageMult = 1.6f;

// ---------------------------------------------------------------------------
// Boss tuning
// ---------------------------------------------------------------------------
constexpr float kBossPhase2HealthFrac = 0.5f;

constexpr float kBruiserRadius = 34.0f;
constexpr float kBruiserHealth = 420.0f;
constexpr float kBruiserSpeed = 110.0f;
constexpr float kBruiserPhase2SpeedMult = 1.35f;
constexpr float kBruiserContactDamage = 22.0f;
constexpr float kBruiserTelegraph = 0.55f;
constexpr float kBruiserChargeSpeed = 640.0f;
constexpr float kBruiserChargeDuration = 0.5f;
constexpr float kBruiserSlamDuration = 0.2f;
constexpr float kBruiserRecover = 0.9f;
constexpr float kBruiserSlamRadius = 170.0f;
constexpr float kBruiserSlamDamage = 34.0f;
constexpr float kBruiserPhase2SlamDamageMult = 1.3f;
constexpr float kBruiserSlamImpulse = 2.6e6f;
constexpr float kBruiserTriggerRange = 240.0f;

constexpr float kCasterRadius = 28.0f;
constexpr float kCasterHealth = 320.0f;
constexpr float kCasterSpeed = 130.0f;
constexpr float kCasterPreferredRange = 380.0f;
constexpr float kCasterVolleyInterval = 1.8f;
constexpr float kCasterPhase2VolleyInterval = 1.0f;
constexpr int   kCasterVolleyCount = 5;
constexpr float kCasterVolleySpreadDeg = 40.0f;
constexpr float kCasterBulletSpeed = 300.0f;
constexpr float kCasterBulletDamage = 10.0f;
constexpr float kCasterBulletRadius = 6.0f;
constexpr float kCasterBulletLife = 2.5f;
constexpr float kCasterSummonInterval = 6.0f; // phase 2 only: summons a minion this often

// ---------------------------------------------------------------------------
// Weapon tuning
// ---------------------------------------------------------------------------
// Celery Sword (melee)
constexpr float kSwordRange = 78.0f;
constexpr float kSwordArcDeg = 110.0f;
constexpr float kSwordDamage = 26.0f;
constexpr float kSwordKnockback = 520.0f;
constexpr float kSwordSwingDuration = 0.18f;
constexpr float kSwordCooldown = 0.38f;

// Churro Blaster (ranged)
constexpr float kBlasterFireRate = 7.0f;   // shots/sec
constexpr float kBlasterSpreadDeg = 6.0f;
constexpr float kBlasterBulletSpeed = 780.0f;
constexpr float kBlasterBulletDamage = 9.0f;
constexpr float kBlasterBulletRadius = 4.0f;
constexpr float kBlasterBulletLife = 1.1f;
constexpr int   kBlasterMagazine = 14;
constexpr float kBlasterReloadTime = 1.1f;
constexpr float kRapidFireCooldown = 0.03f; // Rapid Fire power-up: near-zero time between shots

// Burrito Bomb (throwable AoE)
constexpr float kBombThrowSpeed = 520.0f;
constexpr float kBombFuse = 1.2f;
constexpr float kBombBlastRadius = 130.0f;
constexpr float kBombDamage = 90.0f;
constexpr float kBombImpulseStrength = 3.6e6f;
constexpr float kBombCooldown = 3.2f;
constexpr float kBombRadius = 8.0f;

// Nacho Shield (block + bash)
constexpr float kShieldGuardMax = 100.0f;
constexpr float kShieldDrainPerSec = 55.0f;    // guard meter drain while holding block
constexpr float kShieldRegenPerSec = 30.0f;    // guard meter regen while not blocking
constexpr float kShieldRegenDelay = 0.5f;      // pause after guard breaks/releases before regen starts
constexpr float kShieldBlockMitigation = 0.85f; // fraction of contact damage negated while blocking
constexpr float kShieldBashRange = 56.0f;
constexpr float kShieldBashArcDeg = 100.0f;
constexpr float kShieldBashDamage = 14.0f;
constexpr float kShieldBashKnockback = 640.0f;
constexpr float kShieldBashCooldown = 0.5f;

// ---------------------------------------------------------------------------
// Projectile pool
// ---------------------------------------------------------------------------
constexpr int kMaxProjectiles = 256;

// ---------------------------------------------------------------------------
// Particles
// ---------------------------------------------------------------------------
constexpr int kMaxParticles = 1024;

// ---------------------------------------------------------------------------
// Screen shake
// ---------------------------------------------------------------------------
constexpr float kShakeDecay = 6.0f;

// ---------------------------------------------------------------------------
// Rooms / levels / camera
// ---------------------------------------------------------------------------
// Rooms are deliberately larger than the 1280x720 viewport so the camera
// actually pans while the player crosses one, on top of the hard snap to a
// new room's framing at each cleared gate.
constexpr float kRoomWidth = 1900.0f;
constexpr float kRoomHeight = 1050.0f;
constexpr float kRoomWallMargin = 30.0f; // inset from room edge that entities are clamped to
constexpr float kRoomGateThickness = 32.0f;
constexpr float kLevelIntroDuration = 1.8f;
constexpr float kLevelCompleteDuration = 2.2f;
constexpr int kMaxPickups = 32;
constexpr int kMaxHazards = 8;

// ---------------------------------------------------------------------------
// Pickups
// ---------------------------------------------------------------------------
constexpr float kPickupRadius = 10.0f;
constexpr float kHealthPickupHeal = 25.0f;
constexpr float kPickupDropChance = 0.18f;
constexpr float kPickupLife = 12.0f; // despawns if not collected
constexpr float kPowerUpDropShare = 0.28f; // fraction of drops that are a power-up instead of health
constexpr float kPowerUpDuration = 20.0f;  // Rapid Fire runs a bit shorter, see kRapidFireDuration
constexpr float kRapidFireDuration = 15.0f;
constexpr float kInvincibilityDuration = 8.0f;
constexpr float kBerserkDamageMult = 2.0f;
constexpr float kDoublePointsMult = 2.0f;

// ---------------------------------------------------------------------------
// Difficulty
// ---------------------------------------------------------------------------
constexpr float kDifficultyPromptDuration = 4.0f; // level-1 intro stays up longer to allow a pick

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------
constexpr int kAudioSampleRate = 44100;
constexpr float kDefaultMusicVolume = 0.35f;
constexpr float kDefaultSfxVolume = 0.7f;

// ---------------------------------------------------------------------------
// Hazards
// ---------------------------------------------------------------------------
constexpr float kHazardDamagePerSec = 24.0f;

// ---------------------------------------------------------------------------
// Juice: hit-stop & combo
// ---------------------------------------------------------------------------
constexpr float kHitStopHeavy = 0.05f;   // bomb/boss slam
constexpr float kHitStopMedium = 0.03f;  // sword crit / bash
constexpr float kComboWindow = 3.0f;     // seconds since last kill before the streak resets
constexpr float kComboMultiplierStep = 0.15f; // +15% score per streak step
constexpr float kComboMultiplierCap = 3.0f;

}  // namespace cfg

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------
enum class WeaponType { CelerySword = 0, ChurroBlaster = 1, BurritoBomb = 2, NachoShield = 3, Count };
enum class EnemyType { GlazedChaser, TwistCharger, CheddarShooter, BossBruiser, BossCaster };
enum class GameState { LevelIntro, Playing, LevelComplete, GameOver, Victory };
enum class Difficulty { Easy, Normal, Hard };
enum class PowerUpType { InstaKill, DoublePoints, MaxAmmo, Nuke, RapidFire, Berserk, Invincibility, Count };

// ---------------------------------------------------------------------------
// Difficulty tuning: multipliers applied on top of every base stat. Picked
// once on the level-1 intro banner (see Game::Update) and held for the run.
// ---------------------------------------------------------------------------
struct DifficultyTuning {
    float enemyCountMult = 1.0f;
    float enemyStatMult = 1.0f;   // enemy health & contact damage
    float eliteChanceMult = 1.0f; // >1 = elites show up more often
    float pickupDropMult = 1.0f;
    float scoreMult = 1.0f;
};

inline DifficultyTuning GetDifficultyTuning(Difficulty d) {
    switch (d) {
        case Difficulty::Easy: return DifficultyTuning{0.75f, 0.8f, 0.6f, 1.3f, 0.85f};
        case Difficulty::Hard: return DifficultyTuning{1.35f, 1.25f, 1.7f, 0.75f, 1.3f};
        default: return DifficultyTuning{};
    }
}

inline const char* DifficultyName(Difficulty d) {
    switch (d) {
        case Difficulty::Easy: return "EASY";
        case Difficulty::Hard: return "HARD";
        default: return "NORMAL";
    }
}

inline const char* PowerUpLabel(PowerUpType t) {
    switch (t) {
        case PowerUpType::InstaKill: return "INSTA-KILL";
        case PowerUpType::DoublePoints: return "DOUBLE POINTS";
        case PowerUpType::MaxAmmo: return "MAX AMMO";
        case PowerUpType::Nuke: return "NUKE";
        case PowerUpType::RapidFire: return "RAPID FIRE";
        case PowerUpType::Berserk: return "BERSERK";
        case PowerUpType::Invincibility: return "INVINCIBLE";
        default: return "";
    }
}

inline Color PowerUpColor(PowerUpType t) {
    switch (t) {
        case PowerUpType::InstaKill: return Color{230, 40, 40, 255};
        case PowerUpType::DoublePoints: return Color{250, 210, 40, 255};
        case PowerUpType::MaxAmmo: return Color{90, 200, 230, 255};
        case PowerUpType::Nuke: return Color{160, 255, 90, 255};
        case PowerUpType::RapidFire: return Color{255, 140, 40, 255};
        case PowerUpType::Berserk: return Color{200, 60, 220, 255};
        case PowerUpType::Invincibility: return Color{255, 255, 255, 255};
        default: return WHITE;
    }
}

// ---------------------------------------------------------------------------
// Active timed power-up buffs, shared by Game (which grants them on pickup)
// and Weapon (which checks them when resolving damage/cooldowns). Instant
// power-ups (MaxAmmo, Nuke) don't need a timer here — Game applies them once
// at the moment of collection instead.
// ---------------------------------------------------------------------------
struct PowerUpState {
    float instaKillTimer = 0.0f;
    float doublePointsTimer = 0.0f;
    float rapidFireTimer = 0.0f;
    float berserkTimer = 0.0f;

    void Update(float dt) {
        if (instaKillTimer > 0.0f) instaKillTimer -= dt;
        if (doublePointsTimer > 0.0f) doublePointsTimer -= dt;
        if (rapidFireTimer > 0.0f) rapidFireTimer -= dt;
        if (berserkTimer > 0.0f) berserkTimer -= dt;
    }

    bool InstaKill() const { return instaKillTimer > 0.0f; }
    bool DoublePoints() const { return doublePointsTimer > 0.0f; }
    bool RapidFire() const { return rapidFireTimer > 0.0f; }
    bool Berserk() const { return berserkTimer > 0.0f; }
    bool AnyActive() const { return InstaKill() || DoublePoints() || RapidFire() || Berserk(); }
};

// ---------------------------------------------------------------------------
// Screen shake accumulator, shared by anything that lands a heavy hit
// (melee blows, bomb detonations).
// ---------------------------------------------------------------------------
struct ScreenShake {
    float timer = 0.0f;
    float magnitude = 0.0f;

    void Trigger(float duration, float mag) {
        if (mag >= magnitude || timer <= 0.0f) {
            timer = duration;
            magnitude = mag;
        }
    }

    void Update(float dt) {
        if (timer > 0.0f) {
            timer -= dt;
            if (timer <= 0.0f) { timer = 0.0f; magnitude = 0.0f; }
        }
    }

    Vector2 Offset() const {
        if (timer <= 0.0f) return Vector2{0, 0};
        float t = timer; // decays naturally via Update's countdown
        (void)t;
        float amt = magnitude;
        return Vector2{
            (static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f) * amt,
            (static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f) * amt,
        };
    }
};

// ---------------------------------------------------------------------------
// Kill-streak accumulator shared by every scoring call site (bullet hits,
// sword hits, bomb kills). Each kill within kComboWindow of the last extends
// the streak and raises the score multiplier; letting the window lapse
// resets it.
// ---------------------------------------------------------------------------
struct ComboTracker {
    int streak = 0;
    float windowTimer = 0.0f;
    // External multipliers folded in alongside the streak bonus: difficulty
    // is set once (see Game::ApplyDifficulty), the power-up one tracks
    // Double Points and is refreshed every frame by Game.
    float difficultyScoreMult = 1.0f;
    float powerUpScoreMult = 1.0f;

    void Update(float dt) {
        if (windowTimer > 0.0f) {
            windowTimer -= dt;
            if (windowTimer <= 0.0f) { windowTimer = 0.0f; streak = 0; }
        }
    }

    float Multiplier() const {
        float mult = 1.0f + static_cast<float>(streak) * cfg::kComboMultiplierStep;
        return mult > cfg::kComboMultiplierCap ? cfg::kComboMultiplierCap : mult;
    }

    // Registers a kill and returns the (multiplier-scaled) score to award.
    int RegisterKill(int baseScore) {
        streak++;
        windowTimer = cfg::kComboWindow;
        return static_cast<int>(static_cast<float>(baseScore) * Multiplier() * difficultyScoreMult * powerUpScoreMult);
    }
};

// ---------------------------------------------------------------------------
// Math helpers
// ---------------------------------------------------------------------------
namespace mathutil {

inline float RandomFloat(float lo, float hi) {
    if (hi <= lo) return lo;
    return lo + static_cast<float>(GetRandomValue(0, 100000)) / 100000.0f * (hi - lo);
}

inline float DegToRadF(float deg) { return deg * (PI / 180.0f); }
inline float RadToDegF(float rad) { return rad * (180.0f / PI); }

inline Vector2 FromAngle(float radians, float length = 1.0f) {
    return Vector2{ std::cos(radians) * length, std::sin(radians) * length };
}

inline float AngleOf(Vector2 v) { return std::atan2(v.y, v.x); }

// Shortest signed angular difference (radians) from a to b, in [-PI, PI].
inline float AngleDiff(float a, float b) {
    float d = std::fmod(b - a + PI, 2.0f * PI);
    if (d < 0) d += 2.0f * PI;
    return d - PI;
}

inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// Clamps a position to stay within `bounds`, inset by `radius` on every side.
inline Vector2 ClampToRoom(Vector2 p, float radius, Rectangle bounds) {
    p.x = Clamp(p.x, bounds.x + radius, bounds.x + bounds.width - radius);
    p.y = Clamp(p.y, bounds.y + radius, bounds.y + bounds.height - radius);
    return p;
}

// Inverse-square knockback impulse for a blast at `origin` hitting a point
// at `targetPos` (distance floored to avoid a singularity at the center).
// Shared by BurritoBomb's detonation and Boss slam attacks.
inline Vector2 RadialImpulse(Vector2 targetPos, Vector2 origin, float impulseStrength, float dt) {
    Vector2 diff = Vector2Subtract(targetPos, origin);
    float dist = Vector2Length(diff);
    Vector2 dir = dist > 0.0001f ? Vector2Scale(diff, 1.0f / dist) : Vector2{0, -1};
    float safeDist = std::max(dist, 12.0f);
    float impulseMag = impulseStrength / (safeDist * safeDist);
    return Vector2Scale(dir, impulseMag * dt);
}

// Pick a random point just inside `bounds`, biased to the 4 edges — used to
// spawn enemies around the border of the room the player is currently in.
inline Vector2 RandomEdgePosition(float margin, Rectangle bounds) {
    int edge = GetRandomValue(0, 3);
    float minX = bounds.x + margin, maxX = bounds.x + bounds.width - margin;
    float minY = bounds.y + margin, maxY = bounds.y + bounds.height - margin;
    float x, y;
    switch (edge) {
        case 0: x = RandomFloat(minX, maxX); y = minY; break;
        case 1: x = RandomFloat(minX, maxX); y = maxY; break;
        case 2: x = minX; y = RandomFloat(minY, maxY); break;
        default: x = maxX; y = RandomFloat(minY, maxY); break;
    }
    return Vector2{ x, y };
}

}  // namespace mathutil
