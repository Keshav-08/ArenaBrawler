#include "StoryMode.hpp"
#include "ObstacleRender.hpp"
#include <string>

StoryMode::StoryMode(AudioManager& audio) : audio_(audio) {
    weapons_.push_back(std::make_unique<CelerySword>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
    weapons_.push_back(std::make_unique<ChurroBlaster>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
    weapons_.push_back(std::make_unique<BurritoBomb>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
    weapons_.push_back(std::make_unique<NachoShield>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
    weapons_.push_back(std::make_unique<SkewerSpear>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
    weapons_.push_back(std::make_unique<SalsaScattershot>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
    weapons_.push_back(std::make_unique<HabaneroHandful>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
    weapons_.push_back(std::make_unique<FondueFork>(projectiles_, levels_, particles_, shake_, player_, combo_, pickups_, powerUps_, audio_, score_, floatingText_));
}

void StoryMode::Enter() { RestartGame(); }

void StoryMode::HandleWeaponSwitch() {
    if (IsKeyPressed(KEY_ONE)) currentWeapon_ = 0;
    if (IsKeyPressed(KEY_TWO) && slot2Weapon_ != -1) currentWeapon_ = slot2Weapon_;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && slot2Weapon_ != -1) {
        currentWeapon_ = (currentWeapon_ == 0) ? slot2Weapon_ : 0;
    }
}

void StoryMode::ResolveBulletHits() {
    bool blocking = weapons_[static_cast<size_t>(currentWeapon_)]->IsBlocking();
    const Room& room = levels_.CurrentRoom();
    auto& pool = projectiles_.Pool();
    for (auto& p : pool) {
        if (!p.active || p.kind != ProjectileKind::Bullet) continue;

        // Cover: any bullet (player or hostile) that's flown into a solid
        // obstacle stops there instead of passing through it.
        bool blockedByCover = false;
        for (const Obstacle& obs : room.obstacles) {
            Vector2 obsCenter{room.bounds.x + obs.center.x, room.bounds.y + obs.center.y};
            if (Vector2Distance(p.position, obsCenter) <= p.radius + obs.radius) {
                blockedByCover = true;
                break;
            }
        }
        if (blockedByCover) {
            particles_.SpawnBurst(p.position, 5, Color{170, 170, 170, 255}, 40.0f, 120.0f, 0.1f, 0.2f);
            p.active = false;
            continue;
        }

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
            floatingText_.Spawn(e->position, TextFormat("%.0f", dmg), killed ? GOLD : RAYWHITE);
            Vector2 diff = Vector2Subtract(e->position, p.position);
            if (Vector2LengthSqr(diff) > 0.0001f) {
                e->velocity = Vector2Add(e->velocity, Vector2Scale(Vector2Normalize(diff), 140.0f));
            }
            particles_.SpawnBurst(p.position, 6, Color{255, 220, 140, 255}, 40.0f, 160.0f, 0.1f, 0.25f);
            if (killed) {
                particles_.SpawnBurst(e->position, 14, Color{200, 40, 40, 255}, 60.0f, 260.0f, 0.25f, 0.5f);
                score_ += combo_.RegisterKill(15);
                pickups_.RollAndSpawnDrop(e->position);
                if (e->SplitsOnDeath()) {
                    Vector2 offset = mathutil::FromAngle(mathutil::RandomFloat(0.0f, 2.0f * PI), cfg::kPickleSplitterSplitOffset);
                    levels_.QueueSpawn(EnemyType::PickleSplitter, Vector2Add(e->position, offset), true);
                    levels_.QueueSpawn(EnemyType::PickleSplitter, Vector2Subtract(e->position, offset), true);
                }
                audio_.Play(Sfx::EnemyDeath, 0.7f, 0.15f);
            }
            p.active = false;
            break;
        }
    }
}

void StoryMode::StartLevel(bool advancing) {
    if (advancing) {
        levels_.AdvanceToNextLevel(pickups_);
    } else {
        levels_.StartGame(pickups_);
    }
    player_.position = levels_.LevelStartSpawn();
    player_.velocity = Vector2{0, 0};
    state_ = GameState::LevelIntro;
    // Level 1 holds its intro longer so there's time to pick a difficulty;
    // later levels use the shorter banner since the choice is already locked in.
    stateTimer_ = (!difficultyChosen_) ? cfg::kDifficultyPromptDuration : cfg::kLevelIntroDuration;
}

void StoryMode::ApplyDifficulty(Difficulty d) {
    difficulty_ = d;
    difficultyChosen_ = true;
    DifficultyTuning tuning = GetDifficultyTuning(d);
    levels_.SetDifficulty(d);
    combo_.difficultyScoreMult = tuning.scoreMult;
    pickups_.dropChanceMult = tuning.pickupDropMult;
}

void StoryMode::RestartGame() {
    player_ = Player();
    projectiles_ = ProjectileManager();
    particles_ = ParticleSystem();
    pickups_ = PickupManager();
    shake_ = ScreenShake();
    combo_ = ComboTracker();
    powerUps_ = PowerUpState();
    score_ = 0;
    currentWeapon_ = 0;
    slot2Weapon_ = -1;
    hitStopTimer_ = 0.0f;
    comboPopTimer_ = 0.0f;
    lastStreak_ = 0;
    runTime_ = 0.0f;
    // bestScore_ is deliberately NOT reset here — it survives across
    // restarts for the life of the program (see StoryMode.hpp).
    difficultyChosen_ = false;
    difficulty_ = Difficulty::Normal;
    StartLevel(false);
}

// Resolves "the player walked over a pickup" for every collected item this
// frame: health heals immediately, power-ups grant their buff. Lives here
// (not LevelManager/PickupManager) because it has to reach across Player,
// every Weapon, and LevelManager (Nuke) — exactly the kind of cross-system
// resolution the top-level mode is responsible for.
void StoryMode::ResolvePickupCollection() {
    for (const Pickup& item : pickups_.CollectNear(player_.position, player_.radius)) {
        switch (item.kind) {
            case PickupKind::Health:
                player_.Heal(cfg::kHealthPickupHeal);
                audio_.Play(Sfx::PickupHealth, 1.0f, 0.15f);
                break;

            case PickupKind::Ammo:
                // "Supply crate" flavor: tops off whichever weapon is
                // currently in the loot slot, regardless of which it is.
                if (slot2Weapon_ != -1) weapons_[static_cast<size_t>(slot2Weapon_)]->RefillAndResetCooldown();
                audio_.Play(Sfx::PickupHealth, 1.0f, 0.1f);
                break;

            case PickupKind::Armor:
                player_.EquipArmor(item.armorReduction);
                audio_.Play(Sfx::GateUnlock, 1.0f, -0.1f);
                particles_.SpawnBurst(player_.position, 16, Color{160, 170, 200, 255}, 80.0f, 240.0f, 0.25f, 0.5f);
                break;

            case PickupKind::WeaponLoot: {
                int idx = static_cast<int>(item.weaponLootType);
                slot2Weapon_ = idx;
                currentWeapon_ = idx; // auto-equip what you just picked up, standard loot-game convention
                audio_.Play(Sfx::PickupPowerUp, 1.0f, -0.1f);
                particles_.SpawnBurst(player_.position, 20, WeaponTypeColor(item.weaponLootType), 90.0f, 260.0f, 0.25f, 0.5f);
                break;
            }

            case PickupKind::PowerUp:
                audio_.Play(Sfx::PickupPowerUp, 1.0f, 0.15f);
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
                break;
        }
    }
}

void StoryMode::TriggerHitStop(float duration) {
    if (duration > hitStopTimer_) hitStopTimer_ = duration;
}

Camera2D StoryMode::BuildCamera() const {
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

void StoryMode::Update(float dt) {
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
                if (score_ > bestScore_) bestScore_ = score_;
                audio_.Play(Sfx::Victory);
            }
        }
        return;
    }

    // state_ == Playing
    runTime_ += dt;
    combo_.Update(dt);
    powerUps_.Update(dt);
    combo_.powerUpScoreMult = powerUps_.DoublePoints() ? cfg::kDoublePointsMult : 1.0f;
    if (combo_.streak > lastStreak_) comboPopTimer_ = 0.2f;
    lastStreak_ = combo_.streak;
    if (comboPopTimer_ > 0.0f) comboPopTimer_ -= dt;
    floatingText_.Update(dt);

    Camera2D camera = BuildCamera();
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

    player_.HandleInput(dt, mouseWorld);
    player_.Update(dt, levels_.CurrentRoomPlayArea());
    for (const Obstacle& obs : levels_.CurrentRoom().obstacles) {
        Rectangle roomBounds = levels_.CurrentRoomCameraBounds();
        Vector2 obsCenter{roomBounds.x + obs.center.x, roomBounds.y + obs.center.y};
        player_.position = mathutil::ResolveCircleObstacle(player_.position, player_.radius, obsCenter, obs.radius);
    }
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
        if (score_ > bestScore_) bestScore_ = score_;
        audio_.Play(Sfx::GameOver);
    }
}

namespace {

struct BiomePalette {
    Color floorA, floorB, border;
};

// Distinct per-level terrain look: floor tint + border color. Obstacle kinds
// (see DrawObstacle) are chosen per-room in Level.cpp to match.
BiomePalette GetBiomePalette(Biome biome) {
    switch (biome) {
        case Biome::Grass: return BiomePalette{Color{76, 122, 58, 255}, Color{65, 107, 50, 255}, Color{130, 160, 100, 255}};
        case Biome::Desert: return BiomePalette{Color{198, 172, 118, 255}, Color{183, 155, 100, 255}, Color{225, 200, 150, 255}};
        case Biome::Lava: return BiomePalette{Color{58, 46, 44, 255}, Color{47, 37, 35, 255}, Color{150, 80, 50, 255}};
        case Biome::Ice: return BiomePalette{Color{176, 206, 224, 255}, Color{158, 190, 210, 255}, Color{225, 240, 248, 255}};
        default: return BiomePalette{Color{70, 76, 96, 255}, Color{58, 63, 82, 255}, Color{110, 115, 130, 255}};
    }
}

// A two-tone checkerboard floor, noticeably lighter than the BLACK clear
// color, so a room reads as an actual lit space rather than a void — a flat
// fill (even a lighter one) reads as "black" once it's on screen next to
// true black; real tile contrast is what sells "floor". The palette (and the
// obstacle kinds chosen in Level.cpp) is what actually makes each level's
// terrain read as distinct.
void DrawRoom(const Room& room, bool hasNextRoom, Biome biome) {
    constexpr float kTile = 90.0f;
    BiomePalette palette = GetBiomePalette(biome);
    Color floorA = palette.floorA;
    Color floorB = palette.floorB;

    int startCol = static_cast<int>(std::floor(room.bounds.x / kTile));
    int endCol = static_cast<int>(std::ceil((room.bounds.x + room.bounds.width) / kTile));
    int startRow = static_cast<int>(std::floor(room.bounds.y / kTile));
    int endRow = static_cast<int>(std::ceil((room.bounds.y + room.bounds.height) / kTile));

    for (int row = startRow; row < endRow; ++row) {
        for (int col = startCol; col < endCol; ++col) {
            Rectangle tile{col * kTile, row * kTile, kTile, kTile};
            Rectangle clipped = GetCollisionRec(tile, room.bounds);
            if (clipped.width <= 0.0f || clipped.height <= 0.0f) continue;
            DrawRectangleRec(clipped, ((row + col) % 2 == 0) ? floorA : floorB);
        }
    }

    // Faint accent lines on top of the checkerboard for a bit of grid texture.
    constexpr float kGridStep = kTile;
    Color gridColor = Fade(BLACK, 0.12f);
    for (float gx = room.bounds.x; gx <= room.bounds.x + room.bounds.width; gx += kGridStep) {
        DrawLineV(Vector2{gx, room.bounds.y}, Vector2{gx, room.bounds.y + room.bounds.height}, gridColor);
    }
    for (float gy = room.bounds.y; gy <= room.bounds.y + room.bounds.height; gy += kGridStep) {
        DrawLineV(Vector2{room.bounds.x, gy}, Vector2{room.bounds.x + room.bounds.width, gy}, gridColor);
    }

    DrawRectangleLinesEx(room.bounds, 4.0f, palette.border);

    for (const Hazard& hz : room.hazards) {
        Vector2 center{room.bounds.x + hz.center.x, room.bounds.y + hz.center.y};
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 4.0f);
        DrawCircleV(center, hz.radius + pulse * 4.0f, hz.color);
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), hz.radius, Fade(RED, 0.5f));
    }

    for (const Obstacle& obs : room.obstacles) {
        Vector2 center{room.bounds.x + obs.center.x, room.bounds.y + obs.center.y};
        fx::DrawObstacle(obs, center);
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

void DrawHealthBar(Vector2 pos, float width, float height, float frac, Color fg) {
    DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width), static_cast<int>(height), Fade(DARKGRAY, 0.6f));
    DrawRectangle(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width * mathutil::Clamp01(frac)), static_cast<int>(height), fg);
    DrawRectangleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(width), static_cast<int>(height), RAYWHITE);
}

// Soft red edge vignette that intensifies (and slowly pulses) below ~30% HP
// — same sin(GetTime())-pulse technique already used by the safe-zone
// warning and locked-gate stripes above.
void DrawLowHealthVignette(float healthFrac) {
    constexpr float kThreshold = 0.3f;
    if (healthFrac >= kThreshold) return;
    float severity = 1.0f - (healthFrac / kThreshold); // 0 at threshold, 1 at 0 HP
    float pulse = 0.6f + 0.4f * std::sin(static_cast<float>(GetTime()) * 4.0f);
    float alpha = severity * (0.35f + 0.25f * pulse);
    float edge = 60.0f + severity * 60.0f;
    Color c = ColorAlpha(RED, alpha);
    DrawRectangleGradientV(0, 0, cfg::kScreenWidth, static_cast<int>(edge), c, Fade(c, 0.0f));
    DrawRectangleGradientV(0, cfg::kScreenHeight - static_cast<int>(edge), cfg::kScreenWidth, static_cast<int>(edge), Fade(c, 0.0f), c);
    DrawRectangleGradientH(0, 0, static_cast<int>(edge), cfg::kScreenHeight, c, Fade(c, 0.0f));
    DrawRectangleGradientH(cfg::kScreenWidth - static_cast<int>(edge), 0, static_cast<int>(edge), cfg::kScreenHeight, Fade(c, 0.0f), c);
}

}  // namespace

void StoryMode::Draw() {
    BeginDrawing();
    ClearBackground(BLACK);

    Camera2D camera = BuildCamera();

    BeginMode2D(camera);
    DrawRoom(levels_.CurrentRoom(), levels_.HasNextRoom(), levels_.CurrentBiome());
    if (levels_.HasActiveZone()) {
        Vector2 zoneCenter = levels_.CurrentZoneCenter();
        float zoneRadius = levels_.CurrentZoneRadius();
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 3.0f);
        DrawCircleLines(static_cast<int>(zoneCenter.x), static_cast<int>(zoneCenter.y), zoneRadius, Fade(SKYBLUE, 0.7f));
        DrawCircleLines(static_cast<int>(zoneCenter.x), static_cast<int>(zoneCenter.y), zoneRadius + 3.0f + pulse * 2.0f, Fade(RED, 0.35f));
    }
    levels_.Draw(debugMode_);
    projectiles_.Draw();
    pickups_.Draw();
    particles_.Draw();
    floatingText_.Draw();
    weapons_[static_cast<size_t>(currentWeapon_)]->Draw(player_.position);
    player_.Draw();
    if (debugMode_) {
        DrawLineV(player_.position, Vector2Add(player_.position, player_.velocity), MAGENTA);
        DrawCircleLines(static_cast<int>(player_.position.x), static_cast<int>(player_.position.y), player_.radius, MAGENTA);
    }
    EndMode2D();

    // --- HUD (unaffected by screen shake / camera scroll) ---
    if (state_ == GameState::Playing) DrawLowHealthVignette(player_.health / player_.maxHealth);
    DrawHealthBar(Vector2{20, 20}, 260, 22, player_.health / player_.maxHealth, Color{60, 200, 90, 255});
    DrawText(TextFormat("HP %d/%d", static_cast<int>(player_.health), static_cast<int>(player_.maxHealth)), 28, 22, 16, RAYWHITE);

    DrawText(TextFormat("%s", levels_.LevelName().c_str()), cfg::kScreenWidth / 2 - MeasureText(levels_.LevelName().c_str(), 20) / 2, 20, 20, RAYWHITE);
    DrawText(TextFormat("LEVEL %d/%d   ROOM %d/%d   ENEMIES %d", levels_.LevelNumber(), levels_.LevelCount(),
                        levels_.RoomNumber(), levels_.RoomCount(), levels_.AliveCount()),
              cfg::kScreenWidth / 2 - 130, 46, 16, LIGHTGRAY);

    DrawText(TextFormat("SCORE %06d", score_), cfg::kScreenWidth - 200, 20, 20, RAYWHITE);
    DrawText(TextFormat("%s", DifficultyName(difficulty_)), cfg::kScreenWidth - 200, 44, 14, LIGHTGRAY);
    if (combo_.streak > 1) {
        // Brief scale-pulse on each new kill (comboPopTimer_ counts down from
        // 0.2s) so a growing streak reads as escalating, not a flat counter.
        float popT = mathutil::Clamp01(comboPopTimer_ / 0.2f);
        int fontSize = 14 + static_cast<int>(8.0f * popT);
        const char* text = TextFormat("STREAK x%d  (%.1fx)", combo_.streak, combo_.Multiplier());
        DrawText(text, cfg::kScreenWidth - 200, 62 - static_cast<int>(4.0f * popT), fontSize, GOLD);
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
        std::string bossLabel = levels_.BossName();
        if (levels_.IsFinalLevel()) bossLabel = "FINAL BOSS: " + bossLabel;
        int labelW = MeasureText(bossLabel.c_str(), 18);
        DrawText(bossLabel.c_str(), static_cast<int>(cfg::kScreenWidth / 2.0f - labelW / 2.0f), static_cast<int>(barPos.y - 20), 18,
                 levels_.IsFinalLevel() ? GOLD : RAYWHITE);
        DrawHealthBar(barPos, barWidth, 16, levels_.BossHealthFrac(), Color{210, 60, 60, 255});
    }

    if (levels_.HasActiveZone() && Vector2Distance(player_.position, levels_.CurrentZoneCenter()) > levels_.CurrentZoneRadius()) {
        const char* warn = "OUTSIDE SAFE ZONE";
        int ww = MeasureText(warn, 20);
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 6.0f);
        DrawText(warn, cfg::kScreenWidth / 2 - ww / 2, 100, 20, ColorAlpha(RED, 0.6f + 0.4f * pulse));
    }

    // Dash cooldown indicator.
    DrawText("DASH", 20, 50, 14, LIGHTGRAY);
    DrawHealthBar(Vector2{70, 50}, 100, 12, 1.0f - player_.DashCooldownFrac(), Color{80, 170, 240, 255});

    // Active weapon UI + quick-select hints.
    weapons_[static_cast<size_t>(currentWeapon_)]->DrawUI(Vector2{20, cfg::kScreenHeight - 70.0f});
    if (slot2Weapon_ == -1) {
        DrawText("[2] ---  (find a weapon on the ground)", 220, cfg::kScreenHeight - 70, 16, Fade(LIGHTGRAY, 0.6f));
    }
    std::string hint = slot2Weapon_ == -1
                            ? "[1] Sword  (scroll to cycle once you find a weapon)  [F2] Zombies Mode  [M] Mute"
                            : "[1] Sword  [2] " + std::string(WeaponTypeLabel(static_cast<WeaponType>(slot2Weapon_))) +
                                  "  (scroll to cycle)  [F2] Zombies Mode  [M] Mute";
    DrawText(hint.c_str(), 20, cfg::kScreenHeight - 20, 14, GRAY);

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
            const char* hint2 = "(defaults to NORMAL if you don't choose)";
            int hw = MeasureText(hint2, 14);
            DrawText(hint2, cfg::kScreenWidth / 2 - hw / 2, cfg::kScreenHeight / 2 + 58, 14, LIGHTGRAY);
        }
    }

    if (state_ == GameState::GameOver || state_ == GameState::Victory) {
        bool won = state_ == GameState::Victory;
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.65f));
        const char* msg = won ? "VICTORY!" : "GAME OVER";
        Color msgColor = won ? GOLD : RED;
        int w = MeasureText(msg, 60);
        DrawText(msg, cfg::kScreenWidth / 2 - w / 2, cfg::kScreenHeight / 2 - 130, 60, msgColor);

        int statY = cfg::kScreenHeight / 2 - 50;
        auto drawStat = [&](const char* label, const std::string& value) {
            std::string line = std::string(label) + value;
            int lw = MeasureText(line.c_str(), 22);
            DrawText(line.c_str(), cfg::kScreenWidth / 2 - lw / 2, statY, 22, RAYWHITE);
            statY += 30;
        };
        drawStat("Score: ", std::to_string(score_));
        drawStat("Level Reached: ", std::to_string(levels_.LevelNumber()) + "/" + std::to_string(levels_.LevelCount()));
        int minutes = static_cast<int>(runTime_) / 60;
        int seconds = static_cast<int>(runTime_) % 60;
        drawStat("Time Survived: ", TextFormat("%d:%02d", minutes, seconds));

        std::string bestMsg = score_ >= bestScore_ && score_ > 0 ? "NEW BEST!" : "Best: " + std::to_string(bestScore_);
        Color bestColor = (score_ >= bestScore_ && score_ > 0) ? GOLD : LIGHTGRAY;
        int bw = MeasureText(bestMsg.c_str(), 20);
        DrawText(bestMsg.c_str(), cfg::kScreenWidth / 2 - bw / 2, statY + 8, 20, bestColor);

        const char* hint = won ? "Press R to play again" : "Press R to restart";
        int w3 = MeasureText(hint, 18);
        DrawText(hint, cfg::kScreenWidth / 2 - w3 / 2, statY + 44, 18, LIGHTGRAY);
    }

    EndDrawing();
}
