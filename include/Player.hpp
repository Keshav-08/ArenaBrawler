#pragma once

#include "Common.hpp"
#include "Entity.hpp"

// Player controller: WASD accel/friction movement, mouse aim, dash/roll with
// i-frames, health & score tracking, arena bounds clamping.
class Player : public Entity {
public:
    Player();

    void HandleInput(float dt, Vector2 mouseWorldPos);
    void Update(float dt, Rectangle bounds) override;
    void Draw() const override;

    // Applies contact damage if the player isn't currently invulnerable.
    // Returns true if damage was actually applied.
    bool ApplyContactDamage(float dmg);

    void Heal(float amount) { health = std::min(maxHealth, health + amount); }

    // Invincibility power-up: full damage immunity for `duration` seconds,
    // independent of the dash/contact i-frame timers.
    void GrantInvincibility(float duration) { powerUpInvincibleTimer_ = duration; }
    bool IsPowerUpInvincible() const { return powerUpInvincibleTimer_ > 0.0f; }

    // BossFrost's ice nova: temporarily halves movement speed.
    void ApplySlow(float duration) { slowTimer_ = std::max(slowTimer_, duration); }
    bool IsSlowed() const { return slowTimer_ > 0.0f; }

    // Armor loot: flat damage reduction against everything ApplyContactDamage
    // resolves (contact hits, hostile bullets, boss AoE, hazards). Only
    // upgrades — picking up a weaker tier than what's equipped is a no-op.
    void EquipArmor(float reduction) { armorReduction_ = std::max(armorReduction_, reduction); }
    float ArmorReduction() const { return armorReduction_; }

    bool IsInvulnerable() const {
        return dashIFrameTimer_ > 0.0f || contactIFrameTimer_ > 0.0f || powerUpInvincibleTimer_ > 0.0f;
    }
    bool IsDashing() const { return dashTimer_ > 0.0f; }
    float DashCooldownFrac() const; // 0 = ready, 1 = just used
    float AimAngle() const { return aimAngle_; }

    // One-shot: true for the frame a dash started. Game consumes it to
    // trigger the dash SFX without Player needing to know about audio.
    bool ConsumeJustDashed() {
        bool v = justDashed_;
        justDashed_ = false;
        return v;
    }

private:
    Vector2 moveInput_{0, 0};
    float aimAngle_ = 0.0f;

    float dashTimer_ = 0.0f;
    float dashCooldownTimer_ = 0.0f;
    float dashIFrameTimer_ = 0.0f;
    Vector2 dashDirection_{1, 0};

    float contactIFrameTimer_ = 0.0f;
    float hitFlashTimer_ = 0.0f;
    float powerUpInvincibleTimer_ = 0.0f;
    float slowTimer_ = 0.0f;
    float armorReduction_ = 0.0f;
    bool justDashed_ = false;
};
