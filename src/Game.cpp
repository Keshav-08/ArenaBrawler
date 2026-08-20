#include "Game.hpp"

Game::Game() : storyMode_(audio_), zombiesMode_(audio_) {}

void Game::GetModeButtons(Rectangle& storyBtn, Rectangle& zombiesBtn) const {
    float w = 260.0f, h = 90.0f, gap = 40.0f;
    float totalW = w * 2.0f + gap;
    float left = cfg::kScreenWidth / 2.0f - totalW / 2.0f;
    float top = cfg::kScreenHeight / 2.0f - h / 2.0f;
    storyBtn = Rectangle{left, top, w, h};
    zombiesBtn = Rectangle{left + w + gap, top, w, h};
}

void Game::UpdateModeSelect() {
    Rectangle storyBtn, zombiesBtn;
    GetModeButtons(storyBtn, zombiesBtn);
    Vector2 mouse = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    IGameMode* chosen = nullptr;
    if ((clicked && CheckCollisionPointRec(mouse, storyBtn)) || IsKeyPressed(KEY_ONE)) {
        chosen = &storyMode_;
    } else if ((clicked && CheckCollisionPointRec(mouse, zombiesBtn)) || IsKeyPressed(KEY_TWO)) {
        chosen = &zombiesMode_;
    }
    if (!chosen) return;

    currentMode_ = chosen;
    shellState_ = ShellState::InMode;
    currentMode_->Enter();
}

void Game::DrawModeSelect() const {
    Rectangle storyBtn, zombiesBtn;
    GetModeButtons(storyBtn, zombiesBtn);
    Vector2 mouse = GetMousePosition();
    bool hoverStory = CheckCollisionPointRec(mouse, storyBtn);
    bool hoverZombies = CheckCollisionPointRec(mouse, zombiesBtn);

    BeginDrawing();
    ClearBackground(BLACK);

    const char* title = "ARENA BRAWLER";
    int tw = MeasureText(title, 50);
    DrawText(title, cfg::kScreenWidth / 2 - tw / 2, 150, 50, RAYWHITE);
    const char* subtitle = "Choose a Mode";
    int sw = MeasureText(subtitle, 20);
    DrawText(subtitle, cfg::kScreenWidth / 2 - sw / 2, 212, 20, LIGHTGRAY);

    auto drawButton = [](Rectangle btn, bool hover, const char* label, const char* desc, Color base, Color hoverColor) {
        DrawRectangleRec(btn, hover ? hoverColor : base);
        DrawRectangleLinesEx(btn, hover ? 4.0f : 3.0f, RAYWHITE);
        int lw = MeasureText(label, 24);
        DrawText(label, static_cast<int>(btn.x + btn.width / 2.0f) - lw / 2, static_cast<int>(btn.y + 22.0f), 24, RAYWHITE);
        int dw = MeasureText(desc, 14);
        DrawText(desc, static_cast<int>(btn.x + btn.width / 2.0f) - dw / 2, static_cast<int>(btn.y + 56.0f), 14, LIGHTGRAY);
    };

    drawButton(storyBtn, hoverStory, "[1] STORY MODE", "Levels, bosses, loot", Color{50, 80, 40, 255}, Color{80, 130, 60, 255});
    drawButton(zombiesBtn, hoverZombies, "[2] ZOMBIES MODE", "Infinite survival", Color{80, 40, 40, 255}, Color{130, 60, 60, 255});

    const char* hint = "Click a mode, or press its number key  -  [F2] returns here from either mode";
    int hw = MeasureText(hint, 14);
    DrawText(hint, cfg::kScreenWidth / 2 - hw / 2, cfg::kScreenHeight - 40, 14, GRAY);

    EndDrawing();
}

void Game::Run() {
    InitWindow(cfg::kScreenWidth, cfg::kScreenHeight, cfg::kWindowTitle);
    SetTargetFPS(60);
    audio_.Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (shellState_ == ShellState::InMode && IsKeyPressed(KEY_F2)) {
            currentMode_->Exit();
            currentMode_ = nullptr;
            shellState_ = ShellState::ModeSelect;
        }

        if (shellState_ == ShellState::ModeSelect) {
            UpdateModeSelect();
            DrawModeSelect();
        } else {
            currentMode_->Update(dt);
            currentMode_->Draw();
        }
    }

    audio_.Shutdown();
    CloseWindow();
}
