#pragma once

#include "Audio.hpp"
#include "GameMode.hpp"
#include "StoryMode.hpp"
#include "ZombiesMode.hpp"
#include <memory>

// Top-level entry point: owns the one shared audio device (raylib only
// supports a single audio device, so it can't live inside a mode) and
// whichever IGameMode is currently active. Run() owns the window/main loop
// and is the only place the [F2] mode-switch hotkey is read.
class Game {
public:
    Game();

    void Run();

private:
    void GetModeButtons(Rectangle& storyBtn, Rectangle& zombiesBtn) const;
    void UpdateModeSelect();
    void DrawModeSelect() const;

    AudioManager audio_;
    StoryMode storyMode_;
    ZombiesMode zombiesMode_;
    IGameMode* currentMode_ = nullptr; // null while on the mode-select screen

    enum class ShellState { ModeSelect, InMode };
    ShellState shellState_ = ShellState::ModeSelect;
};
