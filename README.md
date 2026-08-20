# Arena Brawler

A small 2D top-down arena brawler prototype (C++20 + raylib), inspired by
retro arcade brawlers like Hotline Miami. Single arena, three
weapons, escalating enemy waves, no levels/menus — boot straight into wave 1.

## Build

Requires CMake >= 3.20 and a C++20 compiler. raylib 5.0 is fetched
automatically via `FetchContent` on first configure (needs network access and
a few minutes the first time).

```sh
cmake -B build -S .
cmake --build build -j
./build/ArenaBrawler        # Linux/macOS
build\Debug\ArenaBrawler.exe  # Windows (MSVC multi-config)
```

Re-configuring is only needed if `CMakeLists.txt` changes; otherwise
`cmake --build build` picks up new/changed sources under `src/` automatically
(the source list uses `GLOB CONFIGURE_DEPENDS`).

**Apple Silicon note:** if your `cmake` binary itself is x86_64 (e.g. installed
via an Intel Homebrew prefix at `/usr/local` and run under Rosetta — check
with `file $(which cmake)`), it will default this build to x86_64 too, which
still runs fine under Rosetta but isn't native. For a native arm64 build,
configure with `cmake -B build -S . -DCMAKE_OSX_ARCHITECTURES=arm64` instead.

## Controls

| Input            | Action                                   |
|------------------|-------------------------------------------|
| `W A S D`        | Move (accelerates + decelerates smoothly) |
| Mouse            | Aim                                        |
| Left Click       | Attack with the equipped weapon (hold for auto sword/blaster) |
| `Space`          | Dash / roll (brief i-frames, then cooldown) |
| `1` / `2` / `3`  | Equip Celery Sword / Churro Blaster / Burrito Bomb |
| Mouse wheel      | Cycle weapons                              |
| `F1`             | Toggle debug overlay (hitboxes, velocity vectors, FPS) |
| `R`              | Restart after Game Over                    |

## Code style

- C++20, warnings-as-visible (`-Wall -Wextra` / `/W4`), no warnings suppressed.
- No raw `new`/`delete` — enemies live in `std::vector<std::unique_ptr<Enemy>>`;
  everything else (projectiles, particles) is a fixed-capacity pool
  (`std::array`) to avoid per-frame allocation.
- `Entity` is the shared base for anything with position/velocity/HP
  (`Player`, `Enemy` and its AI variants). `Weapon` is a separate hierarchy
  injected with references to the systems it needs (`ProjectileManager`,
  `WaveManager`, `ParticleSystem`, the shared `ScreenShake` accumulator, the
  `Player`, and the running score) so weapons can resolve their own hits
  without `Game` knowing weapon-specific details.
- `Game` is the only class allowed to reach across systems that would
  otherwise create a circular include (e.g. bullet-vs-enemy collision lives
  in `Game::ResolveBulletHits`, not in `Projectile.hpp` or `Enemy.hpp`).
- Tuning constants (speeds, damage, cooldowns, arena bounds) live in
  `Common.hpp` under the `cfg::` namespace — change behavior there, not by
  hardcoding numbers at call sites.

## Architecture notes

- `WaveManager` spawns enemies around the arena border and starts the next
  wave automatically ~1.6s after the roster hits zero, scaling both minion
  types with wave number.
- `BurritoBomb::Update` pops any explosions the `ProjectileManager` produced
  this frame (fuse expiry) and applies radial damage + inverse-square impulse
  to enemies and the player. This runs for *every* weapon each frame
  (`Game::Update` calls `Update` on all three weapons, not just the equipped
  one) so a thrown bomb still detonates after switching weapons.
- `GlazedChaser` uses boids-style separation against other chasers so hordes
  spread out instead of stacking on one point; `TwistCharger` runs a
  Stalk → Telegraph → Charge → Recover state machine with a visual flash
  during telegraph.
