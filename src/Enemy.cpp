#include "Enemy.hpp"

// ---------------------------------------------------------------------------
// Enemy (base)
// ---------------------------------------------------------------------------
Enemy::Enemy(EnemyType type, Vector2 pos, float radius, float health, float speed, float damage)
    : Entity(pos, radius, health), speed(speed), contactDamage(damage), type_(type) {}

void Enemy::Update(float dt, Rectangle bounds) {
    Vector2 moveVelocity = IsSlowed() ? Vector2Scale(velocity, cfg::kSlowSpeedMult) : velocity;
    position = Vector2Add(position, Vector2Scale(moveVelocity, dt));
    position = mathutil::ClampToRoom(position, radius, bounds);
    if (contactCooldownTimer_ > 0.0f) contactCooldownTimer_ -= dt;
    if (markedTimer_ > 0.0f) markedTimer_ -= dt;
    if (slowTimer_ > 0.0f) slowTimer_ -= dt;
}

float Enemy::TryContactDamage() {
    if (contactCooldownTimer_ > 0.0f) return 0.0f;
    contactCooldownTimer_ = cfg::kEnemyContactCooldown;
    return contactDamage;
}

bool Enemy::ConsumeAoeRequest(AoeRequest& out) {
    if (!hasAoeRequest_) return false;
    out = aoeRequest_;
    hasAoeRequest_ = false;
    return true;
}

bool Enemy::ConsumeSummonRequest(SummonRequest& out) {
    if (!hasSummonRequest_) return false;
    out = summonRequest_;
    hasSummonRequest_ = false;
    return true;
}

void Enemy::RequestAoe(Vector2 origin, float radius, float damage, float impulseStrength, float slowDuration) {
    aoeRequest_ = AoeRequest{origin, radius, damage, impulseStrength, slowDuration};
    hasAoeRequest_ = true;
}

void Enemy::RequestSummon(EnemyType type, Vector2 pos) {
    summonRequest_ = SummonRequest{type, pos};
    hasSummonRequest_ = true;
}

void Enemy::MakeElite() {
    elite_ = true;
    radius *= cfg::kEliteRadiusMult;
    health *= cfg::kEliteHealthMult;
    maxHealth = health;
    speed *= cfg::kEliteSpeedMult;
    contactDamage *= cfg::kEliteDamageMult;
}

Color Enemy::EliteTint(Color base) const {
    if (!elite_) return base;
    return Color{
        static_cast<unsigned char>(std::min(255, base.r + 60)),
        static_cast<unsigned char>(std::min(255, base.g + 20)),
        static_cast<unsigned char>(std::min(255, base.b + 60)),
        base.a,
    };
}

void Enemy::DrawEliteRing() const {
    if (!elite_) return;
    float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 6.0f);
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius + 4.0f + pulse * 2.0f,
                     Color{255, 215, 60, 220});
}

void Enemy::Draw() const {
    fx::DrawGroundShadow(position, radius);
    DrawCircleV(position, radius, MAROON);
}

void Enemy::DrawDebug() const {
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, LIME);
    Vector2 velEnd = Vector2Add(position, velocity);
    DrawLineV(position, velEnd, SKYBLUE);
}

// ---------------------------------------------------------------------------
// GlazedChaser
// ---------------------------------------------------------------------------
GlazedChaser::GlazedChaser(Vector2 pos)
    : Enemy(EnemyType::GlazedChaser, pos, cfg::kGlazedChaserRadius, cfg::kGlazedChaserHealth,
            cfg::kGlazedChaserSpeed, cfg::kGlazedChaserDamage) {}

void GlazedChaser::UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                             ProjectileManager& projectiles) {
    (void)dt;
    (void)projectiles;
    Vector2 toPlayer = Vector2Subtract(playerPos, position);
    Vector2 seek = (Vector2LengthSqr(toPlayer) > 0.0001f) ? Vector2Normalize(toPlayer) : Vector2{0, 0};

    // Boids-style separation: push away from nearby same-type neighbours so
    // the horde doesn't clip into a single stacked point.
    Vector2 separation{0, 0};
    for (const auto& other : all) {
        if (other.get() == this || !other->IsAlive()) continue;
        Vector2 diff = Vector2Subtract(position, other->position);
        float dist = Vector2Length(diff);
        if (dist > 0.0001f && dist < cfg::kGlazedChaserSeparationRadius) {
            float strength = (cfg::kGlazedChaserSeparationRadius - dist) / cfg::kGlazedChaserSeparationRadius;
            separation = Vector2Add(separation, Vector2Scale(Vector2Scale(diff, 1.0f / dist), strength));
        }
    }

    Vector2 desired = Vector2Add(Vector2Scale(seek, speed), Vector2Scale(separation, cfg::kGlazedChaserSeparationForce));
    velocity = desired;
}

void GlazedChaser::Draw() const {
    fx::DrawGroundShadow(position, radius);
    DrawCircleV(position, radius, EliteTint(Color{255, 200, 80, 255}));
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.5f));
    if (Vector2LengthSqr(velocity) > 1.0f) {
        Vector2 dir = Vector2Normalize(velocity);
        Vector2 tip = Vector2Add(position, Vector2Scale(dir, radius + 6.0f));
        DrawLineEx(position, tip, 2.0f, BLACK);
    }
    DrawEliteRing();
}

// ---------------------------------------------------------------------------
// TwistCharger
// ---------------------------------------------------------------------------
TwistCharger::TwistCharger(Vector2 pos)
    : Enemy(EnemyType::TwistCharger, pos, cfg::kTwistChargerRadius, cfg::kTwistChargerHealth,
            cfg::kTwistChargerSpeed, cfg::kTwistChargerDamage) {}

void TwistCharger::UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                             ProjectileManager& projectiles) {
    (void)all;
    (void)projectiles;
    stateTimer_ -= dt;
    Vector2 toPlayer = Vector2Subtract(playerPos, position);
    float dist = Vector2Length(toPlayer);

    switch (state_) {
        case State::Stalk: {
            Vector2 dir = (dist > 0.0001f) ? Vector2Normalize(toPlayer) : Vector2{0, 0};
            velocity = Vector2Scale(dir, speed);
            if (dist < cfg::kTwistChargerTriggerRange) {
                state_ = State::Telegraph;
                stateTimer_ = cfg::kTwistChargerTelegraph;
                velocity = Vector2{0, 0};
            }
            break;
        }
        case State::Telegraph: {
            velocity = Vector2{0, 0};
            if (stateTimer_ <= 0.0f) {
                chargeDir_ = (dist > 0.0001f) ? Vector2Normalize(toPlayer) : Vector2{1, 0};
                state_ = State::Charging;
                stateTimer_ = cfg::kTwistChargerChargeDuration;
            }
            break;
        }
        case State::Charging: {
            velocity = Vector2Scale(chargeDir_, cfg::kTwistChargerChargeSpeed);
            if (stateTimer_ <= 0.0f) {
                state_ = State::Recover;
                stateTimer_ = cfg::kTwistChargerRecover;
            }
            break;
        }
        case State::Recover: {
            velocity = Vector2Scale(velocity, 0.85f); // bleed off speed
            if (stateTimer_ <= 0.0f) {
                state_ = State::Stalk;
            }
            break;
        }
    }
}

void TwistCharger::Draw() const {
    fx::DrawGroundShadow(position, radius);
    Color body = Color{150, 90, 200, 255};
    if (state_ == State::Telegraph) {
        // Flash to warn the player a charge is imminent.
        float t = std::fmod(GetTime(), 0.15) < 0.075 ? 1.0f : 0.3f;
        body = ColorAlpha(RED, 0.5f + 0.5f * t);
    } else if (state_ == State::Charging) {
        body = Color{255, 60, 40, 255};
    }
    DrawCircleV(position, radius, EliteTint(body));
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.6f));
    if (Vector2LengthSqr(velocity) > 1.0f) {
        Vector2 dir = Vector2Normalize(velocity);
        Vector2 tip = Vector2Add(position, Vector2Scale(dir, radius + 8.0f));
        DrawLineEx(position, tip, 3.0f, BLACK);
    }
    DrawEliteRing();
}

// ---------------------------------------------------------------------------
// CheddarShooter
// ---------------------------------------------------------------------------
CheddarShooter::CheddarShooter(Vector2 pos)
    : Enemy(EnemyType::CheddarShooter, pos, cfg::kCheddarShooterRadius, cfg::kCheddarShooterHealth,
            cfg::kCheddarShooterSpeed, cfg::kCheddarShooterDamage) {}

void CheddarShooter::UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                               ProjectileManager& projectiles) {
    (void)all;
    Vector2 toPlayer = Vector2Subtract(playerPos, position);
    float dist = Vector2Length(toPlayer);
    Vector2 dir = (dist > 0.0001f) ? Vector2Normalize(toPlayer) : Vector2{0, 0};

    // Hover at kCheddarShooterPreferredRange: flee if the player closes in,
    // approach if they're too far, otherwise strafe to stay mobile.
    float near = cfg::kCheddarShooterPreferredRange - cfg::kCheddarShooterRangeSlop;
    float far = cfg::kCheddarShooterPreferredRange + cfg::kCheddarShooterRangeSlop;
    if (dist < near) {
        velocity = Vector2Scale(dir, -speed);
    } else if (dist > far) {
        velocity = Vector2Scale(dir, speed);
    } else {
        Vector2 strafe{-dir.y, dir.x};
        velocity = Vector2Scale(strafe, speed * 0.6f);
    }

    fireTimer_ -= dt;
    if (fireTimer_ <= 0.0f && dist > 0.0001f) {
        fireTimer_ = cfg::kCheddarShooterFireInterval;
        Vector2 vel = Vector2Scale(dir, cfg::kCheddarShooterBulletSpeed);
        Vector2 spawnPos = Vector2Add(position, Vector2Scale(dir, radius + 4.0f));
        projectiles.SpawnBullet(spawnPos, vel, cfg::kCheddarShooterBulletDamage, cfg::kCheddarShooterBulletRadius,
                                 cfg::kCheddarShooterBulletLife, /*hostile=*/true);
    }
}

void CheddarShooter::Draw() const {
    fx::DrawGroundShadow(position, radius);
    DrawCircleV(position, radius, EliteTint(Color{255, 235, 120, 255}));
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.6f));
    if (Vector2LengthSqr(velocity) > 1.0f) {
        Vector2 dir = Vector2Normalize(velocity);
        Vector2 tip = Vector2Add(position, Vector2Scale(dir, radius + 6.0f));
        DrawLineEx(position, tip, 2.0f, Color{160, 120, 20, 255});
    }
    DrawEliteRing();
}

// ---------------------------------------------------------------------------
// PickleSplitter
// ---------------------------------------------------------------------------
PickleSplitter::PickleSplitter(Vector2 pos, bool isChild)
    : Enemy(EnemyType::PickleSplitter, pos,
            isChild ? cfg::kPickleSplitterRadius * cfg::kPickleSplitterChildSizeMult : cfg::kPickleSplitterRadius,
            isChild ? cfg::kPickleSplitterHealth * cfg::kPickleSplitterChildHealthMult : cfg::kPickleSplitterHealth,
            isChild ? cfg::kPickleSplitterSpeed * cfg::kPickleSplitterChildSpeedMult : cfg::kPickleSplitterSpeed,
            isChild ? cfg::kPickleSplitterDamage * cfg::kPickleSplitterChildDamageMult : cfg::kPickleSplitterDamage),
      isChild_(isChild) {}

void PickleSplitter::UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                               ProjectileManager& projectiles) {
    (void)dt;
    (void)all;
    (void)projectiles;
    Vector2 toPlayer = Vector2Subtract(playerPos, position);
    velocity = Vector2LengthSqr(toPlayer) > 0.0001f ? Vector2Scale(Vector2Normalize(toPlayer), speed) : Vector2{0, 0};
}

void PickleSplitter::Draw() const {
    fx::DrawGroundShadow(position, radius);
    Color body = isChild_ ? Color{150, 205, 95, 255} : Color{90, 160, 60, 255};
    DrawCircleV(position, radius, EliteTint(body));
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.6f));
    DrawCircleV(Vector2{position.x - radius * 0.3f, position.y}, radius * 0.12f, Fade(BLACK, 0.4f));
    DrawCircleV(Vector2{position.x + radius * 0.3f, position.y - radius * 0.2f}, radius * 0.12f, Fade(BLACK, 0.4f));
    DrawEliteRing();
}

// ---------------------------------------------------------------------------
// SodaBomber
// ---------------------------------------------------------------------------
SodaBomber::SodaBomber(Vector2 pos)
    : Enemy(EnemyType::SodaBomber, pos, cfg::kSodaBomberRadius, cfg::kSodaBomberHealth,
            cfg::kSodaBomberSpeed, cfg::kSodaBomberDamage) {}

void SodaBomber::UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                           ProjectileManager& projectiles) {
    (void)all;
    (void)projectiles;
    Vector2 toPlayer = Vector2Subtract(playerPos, position);
    float dist = Vector2Length(toPlayer);

    if (triggered_) {
        velocity = Vector2{0, 0};
        fuseTimer_ -= dt;
        if (fuseTimer_ <= 0.0f) {
            RequestAoe(position, cfg::kSodaBomberBlastRadius, cfg::kSodaBomberBlastDamage, cfg::kSodaBomberBlastImpulse);
            TakeDamage(health + 1.0f); // self-destruct once the fuse runs out
        }
        return;
    }

    if (dist < cfg::kSodaBomberTriggerRange) {
        triggered_ = true;
        fuseTimer_ = cfg::kSodaBomberFuse;
        velocity = Vector2{0, 0};
        return;
    }

    velocity = dist > 0.0001f ? Vector2Scale(Vector2Normalize(toPlayer), speed) : Vector2{0, 0};
}

void SodaBomber::Draw() const {
    fx::DrawGroundShadow(position, radius);
    Color body = Color{200, 70, 50, 255};
    if (triggered_) {
        float t = std::fmod(GetTime(), 0.1) < 0.05 ? 1.0f : 0.4f;
        body = ColorAlpha(RED, 0.5f + 0.5f * t);
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), cfg::kSodaBomberBlastRadius, Fade(ORANGE, 0.35f));
    }
    DrawCircleV(position, radius, EliteTint(body));
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.6f));
    DrawEliteRing();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
std::unique_ptr<Enemy> MakeEnemy(EnemyType type, Vector2 pos, bool elite, float statMult, bool isChild) {
    std::unique_ptr<Enemy> enemy;
    switch (type) {
        case EnemyType::GlazedChaser: enemy = std::make_unique<GlazedChaser>(pos); break;
        case EnemyType::TwistCharger: enemy = std::make_unique<TwistCharger>(pos); break;
        case EnemyType::CheddarShooter: enemy = std::make_unique<CheddarShooter>(pos); break;
        case EnemyType::PickleSplitter: enemy = std::make_unique<PickleSplitter>(pos, isChild); break;
        case EnemyType::SodaBomber: enemy = std::make_unique<SodaBomber>(pos); break;
        default: return nullptr; // bosses are constructed directly by LevelManager, not via this factory
    }
    if (elite) enemy->MakeElite();
    if (statMult != 1.0f) enemy->ScaleStats(statMult);
    return enemy;
}
