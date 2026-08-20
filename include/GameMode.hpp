#pragma once

// Top-level game-mode abstraction. `Game` owns one instance per mode and
// swaps which is active via `[F2]`; each mode is fully self-contained
// (owns its own Player/systems) so switching never leaks state between
// modes — see StoryMode and ZombiesMode.
class IGameMode {
public:
    virtual ~IGameMode() = default;

    // Called once when this mode becomes active (including the very first
    // mode at startup). Expected to fully (re)initialize the mode's state.
    virtual void Enter() = 0;

    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;

    // Called once when switching away from this mode.
    virtual void Exit() = 0;
};
