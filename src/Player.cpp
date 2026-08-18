#include "Player.hpp"

Player::Player() : Entity(Vector2{cfg::kScreenWidth / 2.0f, cfg::kScreenHeight / 2.0f}, cfg::kPlayerRadius, cfg::kPlayerMaxHealth) {}

void Player::HandleInput(float dt, Vector2 mouseWorldPos) {
    (void)dt;
    Vector2 in{0, 0};
    if (IsKeyDown(KEY_W)) in.y -= 1.0f;
    if (IsKeyDown(KEY_S)) in.y += 1.0f;
    if (IsKeyDown(KEY_A)) in.x -= 1.0f;
    if (IsKeyDown(KEY_D)) in.x += 1.0f;
    if (Vector2LengthSqr(in) > 0.0f) in = Vector2Normalize(in);
    moveInput_ = in;

    Vector2 toMouse = Vector2Subtract(mouseWorldPos, position);
    if (Vector2LengthSqr(toMouse) > 0.0001f) {
        aimAngle_ = mathutil::AngleOf(toMouse);
    }

    if (IsKeyPressed(KEY_SPACE) && dashTimer_ <= 0.0f && dashCooldownTimer_ <= 0.0f) {
        Vector2 dir = Vector2LengthSqr(moveInput_) > 0.0f ? moveInput_ : mathutil::FromAngle(aimAngle_);
        dashDirection_ = Vector2Normalize(dir);
        dashTimer_ = cfg::kDashDuration;
        dashIFrameTimer_ = cfg::kDashIFrames;
        dashCooldownTimer_ = cfg::kDashCooldown;
        justDashed_ = true;
    }
}

void Player::Update(float dt, Rectangle bounds) {
    if (dashTimer_ > 0.0f) {
        dashTimer_ -= dt;
        velocity = Vector2Scale(dashDirection_, cfg::kDashSpeed);
    } else {
        // Accelerate towards desired direction, then apply exponential damping (friction).
        Vector2 accel = Vector2Scale(moveInput_, cfg::kPlayerAcceleration);
        velocity = Vector2Add(velocity, Vector2Scale(accel, dt));

        float speed = Vector2Length(velocity);
        if (speed > cfg::kPlayerMaxSpeed) {
            velocity = Vector2Scale(velocity, cfg::kPlayerMaxSpeed / speed);
        }

        float damping = 1.0f / (1.0f + cfg::kPlayerFriction * dt);
        if (Vector2LengthSqr(moveInput_) < 0.0001f) {
            velocity = Vector2Scale(velocity, damping);
        }
    }

    position = Vector2Add(position, Vector2Scale(velocity, dt));
    position = mathutil::ClampToRoom(position, radius, bounds);

    if (dashCooldownTimer_ > 0.0f) dashCooldownTimer_ -= dt;
    if (dashIFrameTimer_ > 0.0f) dashIFrameTimer_ -= dt;
    if (contactIFrameTimer_ > 0.0f) contactIFrameTimer_ -= dt;
    if (hitFlashTimer_ > 0.0f) hitFlashTimer_ -= dt;
    if (powerUpInvincibleTimer_ > 0.0f) powerUpInvincibleTimer_ -= dt;
}

bool Player::ApplyContactDamage(float dmg) {
    if (!alive || IsInvulnerable()) return false;
    TakeDamage(dmg);
    contactIFrameTimer_ = cfg::kContactIFrames;
    hitFlashTimer_ = 0.15f;
    return true;
}

float Player::DashCooldownFrac() const {
    return mathutil::Clamp01(dashCooldownTimer_ / cfg::kDashCooldown);
}

void Player::Draw() const {
    Color body = DARKGREEN;
    if (hitFlashTimer_ > 0.0f) body = RED;
    else if (IsInvulnerable()) body = ColorAlpha(DARKGREEN, 0.55f);

    if (IsPowerUpInvincible()) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 10.0f);
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius + 6.0f + pulse * 3.0f, GOLD);
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius + 10.0f + pulse * 3.0f, Fade(GOLD, 0.5f));
    }

    DrawCircleV(position, radius, body);
    DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), radius, Fade(BLACK, 0.6f));

    // Directional indicator (aim facing).
    Vector2 tip = Vector2Add(position, mathutil::FromAngle(aimAngle_, radius + 10.0f));
    DrawLineEx(position, tip, 3.0f, YELLOW);
}
