#include "Game.hpp"
#include <string>

Game::Game() {
    weapons_.push_back(std::make_unique<CelerySword>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_));
    weapons_.push_back(std::make_unique<ChurroBlaster>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_));
    weapons_.push_back(std::make_unique<BurritoBomb>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_));
    weapons_.push_back(std::make_unique<NachoShield>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_));
    StartLevel(false);
}

void Game::Run() {
    InitWindow(cfg::kScreenWidth, cfg::kScreenHeight, cfg::kWindowTitle);
    SetTargetFPS(60);
    audio_.Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Update(dt);
        Draw();
    }

    audio_.Shutdown();
    CloseWindow();
}

void Game::HandleWeaponSwitch() {
    if (IsKeyPressed(KEY_ONE)) currentWeapon_ = 0;
    if (IsKeyPressed(KEY_TWO)) currentWeapon_ = 1;
    if (IsKeyPressed(KEY_THREE)) currentWeapon_ = 2;
    if (IsKeyPressed(KEY_FOUR)) currentWeapon_ = 3;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        int count = static_cast<int>(weapons_.size());
        currentWeapon_ = ((currentWeapon_ - (wheel > 0 ? 1 : -1)) % count + count) % count;
    }
}

void Game::ResolveBulletHits() {
    bool blocking = weapons_[static_cast<size_t>(currentWeapon_)]->IsBlocking();
    auto& pool = projectiles_.Pool();
    for (auto& p : pool) {
        if (!p.active || p.kind != ProjectileKind::Bullet) continue;

        if (p.hostile) {
            float dist = Vector2Distance(p.position, player_.position);
            if (dist > p.radius + player_.radius) continue;

            float dmg = p.damage;
            if (blocking) dmg *= (1.0f - cfg::kShieldBlockMitigation);
            if (player_.ApplyContactDamage(dmg)) {
                particles_.SpawnBurst(p.position, 8, Color{220, 90, 90, 255}, 60.0f, 220.0f, 0.15f, 0.35f);
                audio_.Play(Sfx::PlayerHurt, 0.7f, 0.15f);
            }
            p.active = false;
            continue;
        }

        for (auto& e : levels_.GetEnemies()) {
            if (!e->IsAlive()) continue;
            float dist = Vector2Distance(p.position, e->position);
            if (dist > p.radius + e->radius) continue;

            float dmg = p.damage;
            if (powerUps_.InstaKill()) dmg = e->health + 1.0f;
            else if (e->IsMarked()) dmg *= cfg::kMarkedDamageMult;

            bool killed = e->TakeDamage(dmg);
            Vector2 diff = Vector2Subtract(e->position, p.position);
            if (Vector2LengthSqr(diff) > 0.0001f) {
                e->velocity = Vector2Add(e->velocity, Vector2Scale(Vector2Normalize(diff), 140.0f));
            }
            particles_.SpawnBurst(p.position, 6, Color{255, 220, 140, 255}, 40.0f, 160.0f, 0.1f, 0.25f);
            if (killed) {
                particles_.SpawnBurst(e->position, 14, Color{200, 40, 40, 255}, 60.0f, 260.0f, 0.25f, 0.5f);
                score_ += combo_.RegisterKill(15);
                pickups_.RollAndSpawnDrop(e->position);
                audio_.Play(Sfx::EnemyDeath, 0.7f, 0.15f);
            }
            p.active = false;
            break;
        }
    }
}

void Game::StartLevel(bool advancing) {
    if (advancing) {
        levels_.AdvanceToNextLevel();
    } else {
        levels_.StartGame();
    }
    player_.position = levels_.LevelStartSpawn();
    player_.velocity = Vector2{0, 0};
    state_ = GameState::LevelIntro;
    // Level 1 holds its intro longer so there's time to pick a difficulty;
    // later levels use the shorter banner since the choice is already locked in.
    stateTimer_ = (!difficultyChosen_) ? cfg::kDifficultyPromptDuration : cfg::kLevelIntroDuration;
}

void Game::ApplyDifficulty(Difficulty d) {
    difficulty_ = d;
    difficultyChosen_ = true;
    DifficultyTuning tuning = GetDifficultyTuning(d);
    levels_.SetDifficulty(d);
    combo_.difficultyScoreMult = tuning.scoreMult;
    pickups_.dropChanceMult = tuning.pickupDropMult;
}

void Game::RestartGame() {
    player_ = Player();
    projectiles_ = ProjectileManager();
    particles_ = ParticleSystem();
    pickups_ = PickupManager();
    shake_ = ScreenShake();
    combo_ = ComboTracker();
    powerUps_ = PowerUpState();
    score_ = 0;
    currentWeapon_ = 0;
    hitStopTimer_ = 0.0f;
    difficultyChosen_ = false;
    difficulty_ = Difficulty::Normal;
    StartLevel(false);
}

// Resolves "the player walked over a pickup" for every collected item this
// frame: health heals immediately, power-ups grant their buff. Lives in Game
// (not LevelManager/PickupManager) because it has to reach across Player,
// every Weapon, and LevelManager (Nuke) — exactly the kind of cross-system
// resolution Game is responsible for.
void Game::ResolvePickupCollection() {
    for (const Pickup& item : pickups_.CollectNear(player_.position, player_.radius)) {
        if (item.kind == PickupKind::Health) {
            player_.Heal(cfg::kHealthPickupHeal);
            audio_.Play(Sfx::PickupHealth);
            continue;
        }

        audio_.Play(Sfx::PickupPowerUp);
        switch (item.powerUpType) {
            case PowerUpType::InstaKill: powerUps_.instaKillTimer = cfg::kPowerUpDuration; break;
            case PowerUpType::DoublePoints: powerUps_.doublePointsTimer = cfg::kPowerUpDuration; break;
            case PowerUpType::RapidFire: powerUps_.rapidFireTimer = cfg::kRapidFireDuration; break;
            case PowerUpType::Berserk: powerUps_.berserkTimer = cfg::kPowerUpDuration; break;
            case PowerUpType::Invincibility: player_.GrantInvincibility(cfg::kInvincibilityDuration); break;
            case PowerUpType::MaxAmmo:
                for (auto& w : weapons_) w->RefillAndResetCooldown();
                break;
            case PowerUpType::Nuke:
                levels_.KillAllEnemies(particles_, combo_, score_);
                shake_.Trigger(0.5f, 16.0f);
                audio_.Play(Sfx::BombExplosion, 1.0f);
                break;
            default: break;
        }
        shake_.Trigger(0.2f, 6.0f);
        particles_.SpawnBurst(player_.position, 24, PowerUpColor(item.powerUpType), 100.0f, 320.0f, 0.3f, 0.6f);
    }
}

void Game::TriggerHitStop(float duration) {
    if (duration > hitStopTimer_) hitStopTimer_ = duration;
}

Camera2D Game::BuildCamera() const {
    Rectangle room = levels_.CurrentRoomCameraBounds();
    Camera2D camera{};
    camera.offset = Vector2Add(Vector2{cfg::kScreenWidth / 2.0f, cfg::kScreenHeight / 2.0f}, shake_.Offset());
    camera.zoom = 1.0f;
    camera.rotation = 0.0f;

    float halfW = cfg::kScreenWidth / 2.0f;
    float halfH = cfg::kScreenHeight / 2.0f;
    float targetX = room.width <= cfg::kScreenWidth
                        ? room.x + room.width * 0.5f
                        : Clamp(player_.position.x, room.x + halfW, room.x + room.width - halfW);
    float targetY = room.height <= cfg::kScreenHeight
                        ? room.y + room.height * 0.5f
                        : Clamp(player_.position.y, room.y + halfH, room.y + room.height - halfH);
    camera.target = Vector2{targetX, targetY};
    return camera;
}

void Game::Update(float dt) {
    audio_.Update(); // keeps the background loop playing regardless of game state
    if (IsKeyPressed(KEY_F1)) debugMode_ = !debugMode_;
    if (IsKeyPressed(KEY_M)) {
        audioMuted_ = !audioMuted_;
        audio_.SetMusicVolume(audioMuted_ ? 0.0f : cfg::kDefaultMusicVolume);
        audio_.SetSfxVolume(audioMuted_ ? 0.0f : cfg::kDefaultSfxVolume);
    }

    if (state_ == GameState::GameOver || state_ == GameState::Victory) {
        if (IsKeyPressed(KEY_R)) RestartGame();
        return;
    }

    if (hitStopTimer_ > 0.0f) {
        hitStopTimer_ -= dt;
        return;
    }

    if (state_ == GameState::LevelIntro || state_ == GameState::LevelComplete) {
        if (state_ == GameState::LevelIntro && !difficultyChosen_) {
            if (IsKeyPressed(KEY_ONE)) ApplyDifficulty(Difficulty::Easy);
            else if (IsKeyPressed(KEY_TWO)) ApplyDifficulty(Difficulty::Normal);
            else if (IsKeyPressed(KEY_THREE)) ApplyDifficulty(Difficulty::Hard);
        }
        stateTimer_ -= dt;
        if (stateTimer_ <= 0.0f) {
            if (state_ == GameState::LevelIntro) {
                if (!difficultyChosen_) ApplyDifficulty(Difficulty::Normal); // default if the player didn't pick
                state_ = GameState::Playing;
            } else if (levels_.HasNextLevel()) {
                StartLevel(true);
            } else {
                state_ = GameState::Victory;
                audio_.Play(Sfx::Victory);
            }
        }
        return;
    }

    // state_ == Playing
    combo_.Update(dt);
    powerUps_.Update(dt);
    combo_.powerUpScoreMult = powerUps_.DoublePoints() ? cfg::kDoublePointsMult : 1.0f;

    Camera2D camera = BuildCamera();
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

    player_.HandleInput(dt, mouseWorld);
    player_.Update(dt, levels_.CurrentRoomPlayArea());
    if (player_.ConsumeJustDashed()) audio_.Play(Sfx::Dash, 0.6f);

    HandleWeaponSwitch();
    Weapon& equipped = *weapons_[static_cast<size_t>(currentWeapon_)];
    equipped.SetBlocking(IsMouseButtonDown(MOUSE_BUTTON_RIGHT));

    Vector2 aimDir = mathutil::FromAngle(player_.AimAngle());
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        equipped.Attack(player_.position, aimDir);
    }

    // Every weapon ticks every frame (not just the equipped one) so that,
    // e.g., a thrown bomb still detonates after the player switches away.
    for (auto& w : weapons_) w->Update(dt);

    projectiles_.Update(dt, levels_.CurrentRoomPlayArea());
    ResolveBulletHits();

    levels_.Update(dt, player_, particles_, projectiles_, shake_, pickups_, audio_, equipped.IsBlocking());
    ResolvePickupCollection();
    pickups_.Update(dt);
    particles_.Update(dt);
    shake_.Update(dt);
    TriggerHitStop(levels_.ConsumeHitStopRequest());

    if (levels_.LevelJustCompleted()) {
        state_ = GameState::LevelComplete;
        stateTimer_ = cfg::kLevelCompleteDuration;
        shake_.Trigger(0.6f, 18.0f);
        audio_.Play(Sfx::LevelComplete);
    }

    if (!player_.IsAlive()) {
        state_ = GameState::GameOver;
        audio_.Play(Sfx::GameOver);
    }
}

// Room floor color is deliberately lighter than the BLACK clear color (and
// the grid on top of it) so a room reads as a bounded space even when the
// camera is too far inside it to see the outer border.
static void DrawRoom(const Room& room, bool hasNextRoom) {
    DrawRectangleRec(room.bounds, Color{40, 43, 54, 255});

    constexpr float kGridStep = 100.0f;
    Color gridColor = Color{56, 60, 74, 255};
    for (float gx = room.bounds.x; gx <= room.bounds.x + room.bounds.width; gx += kGridStep) {
        DrawLineV(Vector2{gx, room.bounds.y}, Vector2{gx, room.bounds.y + room.bounds.height}, gridColor);
    }
    for (float gy = room.bounds.y; gy <= room.bounds.y + room.bounds.height; gy += kGridStep) {
        DrawLineV(Vector2{room.bounds.x, gy}, Vector2{room.bounds.x + room.bounds.width, gy}, gridColor);
    }

    DrawRectangleLinesEx(room.bounds, 4.0f, Color{110, 115, 130, 255});

    for (const Hazard& hz : room.hazards) {
        Vector2 center{room.bounds.x + hz.center.x, room.bounds.y + hz.center.y};
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 4.0f);
        DrawCircleV(center, hz.radius + pulse * 4.0f, hz.color);
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), hz.radius, Fade(RED, 0.5f));
    }

    if (hasNextRoom) {
        float gateX = room.bounds.x + room.bounds.width;
        float half = cfg::kRoomGateThickness * 0.5f;
        Rectangle gateRect{gateX - half, room.bounds.y, cfg::kRoomGateThickness, room.bounds.height};

        if (room.cleared) {
            float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 3.0f);
            DrawRectangleRec(gateRect, Fade(LIME, 0.18f + 0.1f * pulse));
            DrawRectangleLinesEx(gateRect, 3.0f, Fade(LIME, 0.8f));
        } else {
            // Bright pulsing hazard-stripe barrier: unmistakably a locked gate.
            float pulse = 0.55f + 0.45f * std::sin(static_cast<float>(GetTime()) * 6.0f);
            DrawRectangleRec(gateRect, ColorAlpha(RED, 0.35f + 0.25f * pulse));
            DrawRectangleLinesEx(gateRect, 4.0f, ColorAlpha(ORANGE, 0.9f));

            constexpr float kStripeStep = 26.0f;
            constexpr float kStripeWidth = 10.0f;
            float scroll = std::fmod(static_cast<float>(GetTime()) * 30.0f, kStripeStep);
            for (float sy = room.bounds.y - kStripeStep - scroll; sy < room.bounds.y + room.bounds.height + kStripeStep; sy += kStripeStep) {
                Vector2 p1{gateRect.x, sy};
                Vector2 p2{gateRect.x + gateRect.width, sy + kStripeStep};
                DrawLineEx(p1, p2, kStripeWidth, Fade(YELLOW, 0.55f));
            }

            const char* label = "LOCKED - CLEAR THE ROOM";
            int fontSize = 20;
            int textW = MeasureText(label, fontSize);
            Vector2 textPos{gateX - textW / 2.0f, room.bounds.y + room.bounds.height * 0.5f - fontSize * 0.5f};
            DrawRectangle(static_cast<int>(textPos.x) - 8, static_cast<int>(textPos.y) - 4, textW + 16, fontSize + 8, Fade(BLACK, 0.6f));
            DrawText(label, static_cast<int>(textPos.x), static_cast<int>(textPos.y), fontSize, Color{255, 210, 90, 255});
        }
    }
}

static void DrawHealthBar(Vector2 pos, float width, float height, float frac, Color fg) {
    DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width), static_cast<int>(height), Fade(DARKGRAY, 0.6f));
    DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width * mathutil::Clamp01(frac)), static_cast<int>(height), fg);
    DrawRectangleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width), static_cast<int>(height), RAYWHITE);
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(BLACK);

    Camera2D camera = BuildCamera();

    BeginMode2D(camera);
    DrawRoom(levels_.CurrentRoom(), levels_.HasNextRoom());
    levels_.Draw(debugMode_);
    projectiles_.Draw();
    pickups_.Draw();
    particles_.Draw();
    weapons_[static_cast<size_t>(currentWeapon_)]->Draw(player_.position);
    player_.Draw();
    if (debugMode_) {
        DrawLineV(player_.position, Vector2Add(player_.position, player_.velocity), MAGENTA);
        DrawCircleLines(static_cast<int>(player_.position.x), static_cast<int>(player_.position.y), player_.radius, MAGENTA);
    }
    EndMode2D();

    // --- HUD (unaffected by screen shake / camera scroll) ---
    DrawHealthBar(Vector2{20, 20}, 260, 22, player_.health / player_.maxHealth, Color{60, 200, 90, 255});
    DrawText(TextFormat("HP %d/%d", static_cast<int>(player_.health), static_cast<int>(player_.maxHealth)), 28, 22, 16, RAYWHITE);

    DrawText(TextFormat("%s", levels_.LevelName().c_str()), cfg::kScreenWidth / 2 - MeasureText(levels_.LevelName().c_str(), 20) / 2, 20, 20, RAYWHITE);
    DrawText(TextFormat("ROOM %d/%d   ENEMIES %d", levels_.RoomNumber(), levels_.RoomCount(), levels_.AliveCount()),
              cfg::kScreenWidth / 2 - 100, 46, 16, LIGHTGRAY);

    DrawText(TextFormat("SCORE %06d", score_), cfg::kScreenWidth - 200, 20, 20, RAYWHITE);
    DrawText(TextFormat("%s", DifficultyName(difficulty_)), cfg::kScreenWidth - 200, 44, 14, LIGHTGRAY);
    if (combo_.streak > 1) {
        DrawText(TextFormat("STREAK x%d  (%.1fx)", combo_.streak, combo_.Multiplier()), cfg::kScreenWidth - 200, 62, 14, GOLD);
    }

    // Active power-up buffs.
    {
        int y = cfg::kScreenHeight - 100;
        auto drawBuff = [&](bool active, float timer, const char* label, Color c) {
            if (!active) return;
            DrawText(TextFormat("%s %.0fs", label, timer), cfg::kScreenWidth - 200, y, 14, c);
            y -= 18;
        };
        drawBuff(powerUps_.InstaKill(), powerUps_.instaKillTimer, "INSTA-KILL", PowerUpColor(PowerUpType::InstaKill));
        drawBuff(powerUps_.DoublePoints(), powerUps_.doublePointsTimer, "DOUBLE POINTS", PowerUpColor(PowerUpType::DoublePoints));
        drawBuff(powerUps_.RapidFire(), powerUps_.rapidFireTimer, "RAPID FIRE", PowerUpColor(PowerUpType::RapidFire));
        drawBuff(powerUps_.Berserk(), powerUps_.berserkTimer, "BERSERK", PowerUpColor(PowerUpType::Berserk));
        if (player_.IsPowerUpInvincible()) DrawText("INVINCIBLE", cfg::kScreenWidth - 200, y, 14, PowerUpColor(PowerUpType::Invincibility));
    }

    if (levels_.HasActiveBoss()) {
        float barWidth = 480.0f;
        Vector2 barPos{cfg::kScreenWidth / 2.0f - barWidth / 2.0f, 70.0f};
        DrawText(levels_.BossName().c_str(), static_cast<int>(barPos.x), static_cast<int>(barPos.y - 20), 18, RAYWHITE);
        DrawHealthBar(barPos, barWidth, 16, levels_.BossHealthFrac(), Color{210, 60, 60, 255});
    }

    // Dash cooldown indicator.
    DrawText("DASH", 20, 50, 14, LIGHTGRAY);
    DrawHealthBar(Vector2{70, 50}, 100, 12, 1.0f - player_.DashCooldownFrac(), Color{80, 170, 240, 255});

    // Active weapon UI + quick-select hints.
    weapons_[static_cast<size_t>(currentWeapon_)]->DrawUI(Vector2{20, cfg::kScreenHeight - 70.0f});
    DrawText("[1] Sword  [2] Blaster  [3] Bomb  [4] Shield  (scroll to cycle)  [M] Mute", 20, cfg::kScreenHeight - 20, 14, GRAY);

    if (debugMode_) {
        DrawFPS(cfg::kScreenWidth - 100, cfg::kScreenHeight - 24);
        DrawText("DEBUG MODE (F1)", cfg::kScreenWidth - 220, cfg::kScreenHeight - 46, 14, YELLOW);
    }

    if (state_ == GameState::LevelIntro || state_ == GameState::LevelComplete) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.55f));
        const char* headline = state_ == GameState::LevelIntro ? levels_.LevelName().c_str() : "LEVEL CLEAR";
        int w = MeasureText(headline, 44);
        DrawText(headline, cfg::kScreenWidth / 2 - w / 2, cfg::kScreenHeight / 2 - 30, 44, RAYWHITE);

        if (state_ == GameState::LevelIntro && !difficultyChosen_) {
            const char* prompt = "CHOOSE DIFFICULTY:   [1] EASY   [2] NORMAL   [3] HARD";
            int pw = MeasureText(prompt, 22);
            DrawText(prompt, cfg::kScreenWidth / 2 - pw / 2, cfg::kScreenHeight / 2 + 30, 22, GOLD);
            const char* hint = "(defaults to NORMAL if you don't choose)";
            int hw = MeasureText(hint, 14);
            DrawText(hint, cfg::kScreenWidth / 2 - hw / 2, cfg::kScreenHeight / 2 + 58, 14, LIGHTGRAY);
        }
    }

    if (state_ == GameState::GameOver) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.65f));
        const char* msg = "GAME OVER";
        int w = MeasureText(msg, 60);
        DrawText(msg, cfg::kScreenWidth / 2 - w / 2, cfg::kScreenHeight / 2 - 60, 60, RED);
        std::string scoreMsg = "Final Score: " + std::to_string(score_);
        int w2 = MeasureText(scoreMsg.c_str(), 24);
        DrawText(scoreMsg.c_str(), cfg::kScreenWidth / 2 - w2 / 2, cfg::kScreenHeight / 2 + 10, 24, RAYWHITE);
        const char* hint = "Press R to restart";
        int w3 = MeasureText(hint, 18);
        DrawText(hint, cfg::kScreenWidth / 2 - w3 / 2, cfg::kScreenHeight / 2 + 46, 18, LIGHTGRAY);
    }

    if (state_ == GameState::Victory) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.65f));
        const char* msg = "VICTORY!";
        int w = MeasureText(msg, 60);
        DrawText(msg, cfg::kScreenWidth / 2 - w / 2, cfg::kScreenHeight / 2 - 60, 60, GOLD);
        std::string scoreMsg = "Final Score: " + std::to_string(score_);
        int w2 = MeasureText(scoreMsg.c_str(), 24);
        DrawText(scoreMsg.c_str(), cfg::kScreenWidth / 2 - w2 / 2, cfg::kScreenHeight / 2 + 10, 24, RAYWHITE);
        const char* hint = "Press R to play again";
        int w3 = MeasureText(hint, 18);
        DrawText(hint, cfg::kScreenWidth / 2 - w3 / 2, cfg::kScreenHeight / 2 + 46, 18, LIGHTGRAY);
    }

    EndDrawing();
}
