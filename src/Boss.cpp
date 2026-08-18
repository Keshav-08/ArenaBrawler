#include "Boss.hpp"

// ---------------------------------------------------------------------------
// Boss (base)
// ---------------------------------------------------------------------------
Boss::Boss(EnemyType type, std::string name, Vector2 pos, float radius, float health, float speed, float damage)
    : Enemy(type, pos, radius, health, speed, damage), name_(std::move(name)) {}

bool Boss::ConsumeAoeRequest(AoeRequest& out) {
    if (!hasAoeRequest_) return false;
    out = aoeRequest_;
    hasAoeRequest_ = false;
    return true;
}

bool Boss::ConsumeSummonRequest(SummonRequest& out) {
    if (!hasSummonRequest_) return false;
    out = summonRequest_;
    hasSummonRequest_ = false;
    return true;
}

void Boss::RequestAoe(Vector2 origin, float radius, float damage, float impulseStrength) {
    aoeRequest_ = AoeRequest{origin, radius, damage, impulseStrength};
    hasAoeRequest_ = true;
}

void Boss::RequestSummon(EnemyType type, Vector2 pos) {
    summonRequest_ = SummonRequest{type, pos};
    hasSummonRequest_ = true;
}

// ---------------------------------------------------------------------------
// BossBruiser
// ---------------------------------------------------------------------------
BossBruiser::BossBruiser(Vector2 pos)
    : Boss(EnemyType::BossBruiser, "Sir Loin", pos, cfg::kBruiserRadius, cfg::kBruiserHealth,
           cfg::kBruiserSpeed, cfg::kBruiserContactDamage) {}

void BossBruiser::UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                            ProjectileManager& projectiles) {
    (void)all;
    (void)projectiles;
    stateTimer_ -= dt;
    Vector2 toPlayer = Vector2Subtract(playerPos, position);
    float dist = Vector2Length(toPlayer);
    float speedMult = InPhase2() ? cfg::kBruiserPhase2SpeedMult : 1.0f;

    switch (state_) {
        case State::Stalk: {
            Vector2 dir = (dist > 0.0001f) ? Vector2Normalize(toPlayer) : Vector2{0, 0};
            velocity = Vector2Scale(dir, speed * speedMult);
            if (dist < cfg::kBruiserTriggerRange) {
                state_ = State::Telegraph;
                stateTimer_ = cfg::kBruiserTelegraph;
                velocity = Vector2{0, 0};
            }
            break;
        }
        case State::Telegraph: {
            velocity = Vector2{0, 0};
            if (stateTimer_ <= 0.0f) {
                chargeDir_ = (dist > 0.0001f) ? Vector2Normalize(toPlayer) : Vector2{1, 0};
                state_ = State::Charging;
                stateTimer_ = cfg::kBruiserChargeDuration;
            }
            break;
        }
        case State::Charging: {
            velocity = Vector2Scale(chargeDir_, cfg::kBruiserChargeSpeed * speedMult);
            if (stateTimer_ <= 0.0f) {
                state_ = State::Slam;
                stateTimer_ = cfg::kBruiserSlamDuration;
                velocity = Vector2{0, 0};
                float dmg = cfg::kBruiserSlamDamage * (InPhase2() ? cfg::kBruiserPhase2SlamDamageMult : 1.0f);
                RequestAoe(position, cfg::kBruiserSlamRadius, dmg, cfg::kBruiserSlamImpulse);
            }
            break;
        }
        case State::Slam: {
            velocity = Vector2{0, 0};
            if (stateTimer_ <= 0.0f) {
                state_ = State::Recover;
                stateTimer_ = cfg::kBruiserRecover;
            }
            break;
        }
        case State::Recover: {
            velocity = Vector2Scale(velocity, 0.85f);
            if (stateTimer_ <= 0.0f) {
                state_ = State::Stalk;
            }
            break;
        }
    }
}

void BossBruiser::Draw() const {
    Color body = InPhase2() ? Color{180, 40, 30, 255} : Color{120, 60, 40, 255};
    if (state_ == State::Telegraph) {
        float t = std::fmod(GetTime(), 0.12) < 0.06 ? 1.0f : 0.35f;
        body = ColorAlpha(RED, 0.5f + 0.5f * t);
    } else if (state_ == State::Charging) {
        body = Color{255, 80, 30, 255};
    } else if (state_ == State::Slam) {
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), cfg::kBruiserSlamRadius,
                         Fade(ORANGE, 0.7f));
    }
    DrawCircleV(position, radius, body);
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.7f));
    if (Vector2LengthSqr(velocity) > 1.0f) {
        Vector2 dir = Vector2Normalize(velocity);
        Vector2 tip = Vector2Add(position, Vector2Scale(dir, radius + 10.0f));
        DrawLineEx(position, tip, 4.0f, BLACK);
    }
}

// ---------------------------------------------------------------------------
// BossCaster
// ---------------------------------------------------------------------------
BossCaster::BossCaster(Vector2 pos)
    : Boss(EnemyType::BossCaster, "The Casserole", pos, cfg::kCasterRadius, cfg::kCasterHealth,
           cfg::kCasterSpeed, cfg::kCheddarShooterDamage) {}

void BossCaster::UpdateAI(float dt, Vector2 playerPos, const std::vector<std::unique_ptr<Enemy>>& all,
                           ProjectileManager& projectiles) {
    (void)all;
    Vector2 toPlayer = Vector2Subtract(playerPos, position);
    float dist = Vector2Length(toPlayer);
    Vector2 dir = (dist > 0.0001f) ? Vector2Normalize(toPlayer) : Vector2{0, 0};

    float near = cfg::kCasterPreferredRange - 50.0f;
    float far = cfg::kCasterPreferredRange + 50.0f;
    if (dist < near) {
        velocity = Vector2Scale(dir, -speed);
    } else if (dist > far) {
        velocity = Vector2Scale(dir, speed);
    } else {
        Vector2 strafe{-dir.y, dir.x};
        velocity = Vector2Scale(strafe, speed * 0.5f);
    }

    float volleyInterval = InPhase2() ? cfg::kCasterPhase2VolleyInterval : cfg::kCasterVolleyInterval;
    volleyTimer_ -= dt;
    if (volleyTimer_ <= 0.0f && dist > 0.0001f) {
        volleyTimer_ = volleyInterval;
        float baseAngle = mathutil::AngleOf(dir);
        float halfSpread = mathutil::DegToRadF(cfg::kCasterVolleySpreadDeg) * 0.5f;
        for (int i = 0; i < cfg::kCasterVolleyCount; ++i) {
            float t = cfg::kCasterVolleyCount > 1 ? static_cast<float>(i) / (cfg::kCasterVolleyCount - 1) : 0.5f;
            float angle = baseAngle - halfSpread + 2.0f * halfSpread * t;
            Vector2 vel = mathutil::FromAngle(angle, cfg::kCasterBulletSpeed);
            Vector2 spawnPos = Vector2Add(position, mathutil::FromAngle(angle, radius + 4.0f));
            projectiles.SpawnBullet(spawnPos, vel, cfg::kCasterBulletDamage, cfg::kCasterBulletRadius,
                                     cfg::kCasterBulletLife, /*hostile=*/true);
        }
    }

    if (InPhase2()) {
        summonTimer_ -= dt;
        if (summonTimer_ <= 0.0f) {
            summonTimer_ = cfg::kCasterSummonInterval;
            Vector2 offset = mathutil::FromAngle(mathutil::RandomFloat(0.0f, 2.0f * PI), 80.0f);
            RequestSummon(EnemyType::GlazedChaser, Vector2Add(position, offset));
        }
    }
}

void BossCaster::Draw() const {
    Color body = InPhase2() ? Color{200, 60, 160, 255} : Color{110, 70, 170, 255};
    DrawCircleV(position, radius, body);
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.7f));
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius + 10.0f, Fade(body, 0.4f));
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
std::unique_ptr<Enemy> MakeBoss(EnemyType type, Vector2 pos, float statMult) {
    std::unique_ptr<Enemy> boss;
    switch (type) {
        case EnemyType::BossBruiser: boss = std::make_unique<BossBruiser>(pos); break;
        case EnemyType::BossCaster: boss = std::make_unique<BossCaster>(pos); break;
        default: return nullptr;
    }
    if (statMult != 1.0f) boss->ScaleStats(statMult);
    return boss;
}
