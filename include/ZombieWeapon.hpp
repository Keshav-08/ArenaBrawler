#pragma once

#include "raylib.h"

// Zombies Mode's own small weapon system. Deliberately separate from
// Weapon.hpp/WeaponType: the story-mode Weapon hierarchy is constructed
// with a LevelManager& and resolves hits against levels_.GetEnemies()
// directly, so it isn't reusable by a mode that has no LevelManager.
// Rather than refactor that shared class (risking the existing campaign)
// this mode hand-rolls its own hit resolution — see ZombiesMode.cpp —
// using just these stats as tuning data.
//
// FireMode is the axis that lets ~30 guns share a handful of resolution
// paths instead of needing one-off code per weapon: most guns are just a
// stats row against an existing mode. Only Laser, Chain, and Freeze need
// genuinely new resolution code.
enum class FireMode {
    Melee,      // swept-arc hit test
    SemiAuto,   // one bullet per click
    FullAuto,   // continuous bullets while held
    Burst,      // burstCount bullets, burstDelay apart, per click
    Shotgun,    // pelletCount bullets per click, spread across spreadDeg
    Explosive,  // thrown/launched, AoE (blastRadius) on impact
    Continuous, // damage-per-second forward cone while held (no ammo)
    Laser,      // instant line hit out to range while held (no travel time)
    Chain,      // hits nearest target, jumps to nearby targets chainCount times
    Freeze,     // like SemiAuto/Explosive but also applies Enemy::ApplySlow
};

enum class ZombieWeaponKind {
    // Melee
    Sword, RollingPin,
    // SemiAuto
    PeppermintPopper, SaltshakerSixShooter, FondueFiftyCal, LongRibRifle,
    // FullAuto
    Blaster, NachoNinja, PretzelStorm, BaguetteBattleRifle, WaffleIronAR,
    TurkeyLegTurretGun, MeatloafMinigun, GravyGatling,
    // Burst
    CornbreadCarbine, ChiliChopper,
    // Shotgun
    BurritoBoomstick, PizzaSlicer, SausageSweeper, DrumstickDoubleBarrel,
    // Explosive
    Bomb, PumpkinPieLauncher, CandyCaneRPG, PopcornClusterGrenade,
    // Continuous
    Flamethrower, PickleJuiceSprayer,
    // Laser
    LemonadeLaser, SkewerSniper,
    // Chain
    SodaPopTesla,
    // Freeze
    SundaeStormCannon,
    Count,
};

struct ZombieWeaponStats {
    const char* name = "";
    FireMode fireMode = FireMode::SemiAuto;
    float damage = 0.0f;      // per-shot (or per-second for Continuous/Laser, per-pellet for Shotgun)
    float cooldown = 0.0f;    // time between shots/bursts/melee swings
    float range = 0.0f;       // Melee/Continuous/Laser reach; unused by bullet-based modes
    float arcDeg = 90.0f;     // Melee/Continuous cone width
    int wallBuyCost = 0;
    int ammoRefillCost = 0;
    int magazineSize = 0;     // 0 = no ammo tracking (melee, continuous, chain)
    int burstCount = 1;
    float burstDelay = 0.0f;
    int pelletCount = 1;
    float spreadDeg = 0.0f;
    float blastRadius = 0.0f;
    float bulletSpeed = 800.0f;
    int chainCount = 1;
    float chainRadius = 0.0f;
    float slowDuration = 0.0f;
};

inline const ZombieWeaponStats& GetZombieWeaponStats(ZombieWeaponKind kind) {
    static const ZombieWeaponStats table[static_cast<size_t>(ZombieWeaponKind::Count)] = {
        // --- Melee ---
        {.name = "Celery Sword", .fireMode = FireMode::Melee, .damage = 45.0f, .cooldown = 0.35f, .range = 80.0f, .arcDeg = 100.0f},
        {.name = "Rolling Pin Reaper", .fireMode = FireMode::Melee, .damage = 70.0f, .cooldown = 0.55f, .range = 85.0f, .arcDeg = 90.0f, .wallBuyCost = 800},

        // --- SemiAuto ---
        {.name = "Peppermint Popper", .fireMode = FireMode::SemiAuto, .damage = 22.0f, .cooldown = 0.25f, .wallBuyCost = 350, .ammoRefillCost = 150, .magazineSize = 18, .bulletSpeed = 900.0f},
        {.name = "Saltshaker Six-Shooter", .fireMode = FireMode::SemiAuto, .damage = 35.0f, .cooldown = 0.4f, .wallBuyCost = 600, .ammoRefillCost = 200, .magazineSize = 10, .bulletSpeed = 900.0f},
        {.name = "Fondue Fifty-Cal", .fireMode = FireMode::SemiAuto, .damage = 140.0f, .cooldown = 1.1f, .wallBuyCost = 1800, .ammoRefillCost = 400, .magazineSize = 7, .bulletSpeed = 1200.0f},
        {.name = "Long Rib Rifle", .fireMode = FireMode::SemiAuto, .damage = 90.0f, .cooldown = 0.7f, .wallBuyCost = 1400, .ammoRefillCost = 350, .magazineSize = 12, .bulletSpeed = 1100.0f},

        // --- FullAuto ---
        {.name = "Churro Blaster", .fireMode = FireMode::FullAuto, .damage = 9.0f, .cooldown = 1.0f / 7.0f, .wallBuyCost = 500, .ammoRefillCost = 250, .magazineSize = 36, .spreadDeg = 3.0f, .bulletSpeed = 780.0f},
        {.name = "Nacho Ninja", .fireMode = FireMode::FullAuto, .damage = 7.0f, .cooldown = 1.0f / 12.0f, .wallBuyCost = 700, .ammoRefillCost = 250, .magazineSize = 48, .spreadDeg = 4.0f, .bulletSpeed = 800.0f},
        {.name = "Pretzel Storm", .fireMode = FireMode::FullAuto, .damage = 8.0f, .cooldown = 0.1f, .wallBuyCost = 750, .ammoRefillCost = 260, .magazineSize = 45, .spreadDeg = 5.0f, .bulletSpeed = 800.0f},
        {.name = "Baguette Battle Rifle", .fireMode = FireMode::FullAuto, .damage = 16.0f, .cooldown = 0.125f, .wallBuyCost = 1100, .ammoRefillCost = 300, .magazineSize = 38, .spreadDeg = 3.0f, .bulletSpeed = 850.0f},
        {.name = "Waffle Iron AR", .fireMode = FireMode::FullAuto, .damage = 14.0f, .cooldown = 1.0f / 9.0f, .wallBuyCost = 1000, .ammoRefillCost = 300, .magazineSize = 42, .spreadDeg = 3.0f, .bulletSpeed = 850.0f},
        {.name = "Turkey Leg Turret-Gun", .fireMode = FireMode::FullAuto, .damage = 13.0f, .cooldown = 1.0f / 11.0f, .wallBuyCost = 1600, .ammoRefillCost = 450, .magazineSize = 100, .spreadDeg = 6.0f, .bulletSpeed = 800.0f},
        {.name = "Meatloaf Minigun", .fireMode = FireMode::FullAuto, .damage = 11.0f, .cooldown = 1.0f / 15.0f, .wallBuyCost = 2200, .ammoRefillCost = 550, .magazineSize = 150, .spreadDeg = 8.0f, .bulletSpeed = 780.0f},
        {.name = "Gravy Gatling", .fireMode = FireMode::FullAuto, .damage = 18.0f, .cooldown = 0.1f, .wallBuyCost = 1900, .ammoRefillCost = 500, .magazineSize = 90, .spreadDeg = 5.0f, .bulletSpeed = 800.0f},

        // --- Burst ---
        {.name = "Cornbread Carbine", .fireMode = FireMode::Burst, .damage = 20.0f, .cooldown = 0.5f, .wallBuyCost = 900, .ammoRefillCost = 280, .magazineSize = 36, .burstCount = 3, .burstDelay = 0.06f, .bulletSpeed = 850.0f},
        {.name = "Chili Chopper", .fireMode = FireMode::Burst, .damage = 14.0f, .cooldown = 0.4f, .wallBuyCost = 850, .ammoRefillCost = 260, .magazineSize = 45, .burstCount = 2, .burstDelay = 0.05f, .bulletSpeed = 820.0f},

        // --- Shotgun ---
        {.name = "Burrito Boomstick", .fireMode = FireMode::Shotgun, .damage = 18.0f, .cooldown = 0.8f, .wallBuyCost = 750, .ammoRefillCost = 220, .magazineSize = 10, .pelletCount = 8, .spreadDeg = 22.0f, .bulletSpeed = 700.0f},
        {.name = "Pizza Slicer", .fireMode = FireMode::Shotgun, .damage = 14.0f, .cooldown = 0.9f, .wallBuyCost = 950, .ammoRefillCost = 250, .magazineSize = 12, .pelletCount = 10, .spreadDeg = 26.0f, .bulletSpeed = 680.0f},
        {.name = "Sausage Sweeper", .fireMode = FireMode::Shotgun, .damage = 22.0f, .cooldown = 0.7f, .wallBuyCost = 1050, .ammoRefillCost = 260, .magazineSize = 10, .pelletCount = 6, .spreadDeg = 18.0f, .bulletSpeed = 720.0f},
        {.name = "Drumstick Double-Barrel", .fireMode = FireMode::Shotgun, .damage = 35.0f, .cooldown = 1.0f, .wallBuyCost = 1200, .ammoRefillCost = 200, .magazineSize = 4, .pelletCount = 4, .spreadDeg = 14.0f, .bulletSpeed = 750.0f},

        // --- Explosive ---
        {.name = "Burrito Bomb", .fireMode = FireMode::Explosive, .damage = 90.0f, .cooldown = 3.0f, .wallBuyCost = 1200, .blastRadius = 130.0f, .bulletSpeed = 520.0f},
        {.name = "Pumpkin Pie Launcher", .fireMode = FireMode::Explosive, .damage = 120.0f, .cooldown = 2.2f, .wallBuyCost = 1700, .ammoRefillCost = 400, .magazineSize = 6, .blastRadius = 150.0f, .bulletSpeed = 600.0f},
        {.name = "Candy Cane RPG", .fireMode = FireMode::Explosive, .damage = 180.0f, .cooldown = 3.5f, .wallBuyCost = 2500, .ammoRefillCost = 500, .magazineSize = 3, .blastRadius = 170.0f, .bulletSpeed = 700.0f},
        {.name = "Popcorn Cluster Grenade", .fireMode = FireMode::Explosive, .damage = 60.0f, .cooldown = 2.0f, .wallBuyCost = 1300, .blastRadius = 200.0f, .bulletSpeed = 480.0f},

        // --- Continuous ---
        {.name = "Sriracha Flamethrower", .fireMode = FireMode::Continuous, .damage = 40.0f, .range = 110.0f, .arcDeg = 70.0f, .wallBuyCost = 1500},
        {.name = "Pickle Juice Sprayer", .fireMode = FireMode::Continuous, .damage = 32.0f, .range = 130.0f, .arcDeg = 60.0f, .wallBuyCost = 1650},

        // --- Laser ---
        {.name = "Lemonade Laser", .fireMode = FireMode::Laser, .damage = 55.0f, .range = 260.0f, .wallBuyCost = 2000},
        {.name = "Skewer Sniper", .fireMode = FireMode::Laser, .damage = 90.0f, .range = 400.0f, .wallBuyCost = 2600},

        // --- Chain ---
        {.name = "Soda Pop Tesla", .fireMode = FireMode::Chain, .damage = 50.0f, .cooldown = 0.6f, .wallBuyCost = 2200, .chainCount = 4, .chainRadius = 140.0f},

        // --- Freeze ---
        {.name = "Sundae Storm Cannon", .fireMode = FireMode::Freeze, .damage = 45.0f, .cooldown = 1.2f, .wallBuyCost = 2800, .ammoRefillCost = 500, .magazineSize = 16, .bulletSpeed = 650.0f, .slowDuration = 2.5f},
    };
    return table[static_cast<size_t>(kind)];
}
