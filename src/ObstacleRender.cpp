#include "ObstacleRender.hpp"

namespace fx {

void DrawObstacle(const Obstacle& obs, Vector2 center) {
    fx::DrawGroundShadow(center, obs.radius);
    switch (obs.kind) {
        case ObstacleKind::Crate: {
            Rectangle crate{center.x - obs.radius, center.y - obs.radius, obs.radius * 2.0f, obs.radius * 2.0f};
            DrawRectangleRec(crate, Color{120, 84, 52, 255});
            DrawRectangleLinesEx(crate, 3.0f, Color{70, 48, 28, 255});
            DrawLineEx(Vector2{crate.x, crate.y}, Vector2{crate.x + crate.width, crate.y + crate.height}, 3.0f, Color{70, 48, 28, 255});
            DrawLineEx(Vector2{crate.x + crate.width, crate.y}, Vector2{crate.x, crate.y + crate.height}, 3.0f, Color{70, 48, 28, 255});
            break;
        }
        case ObstacleKind::Tree: {
            DrawRectangle(static_cast<int>(center.x - obs.radius * 0.18f), static_cast<int>(center.y - obs.radius * 0.2f),
                          static_cast<int>(obs.radius * 0.36f), static_cast<int>(obs.radius * 1.2f), Color{90, 62, 40, 255});
            DrawCircleV(Vector2{center.x, center.y - obs.radius * 0.6f}, obs.radius, Color{50, 120, 55, 255});
            DrawCircleV(Vector2{center.x - obs.radius * 0.4f, center.y - obs.radius * 0.3f}, obs.radius * 0.7f, Color{60, 135, 62, 255});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y - obs.radius * 0.6f), obs.radius, Fade(BLACK, 0.4f));
            break;
        }
        case ObstacleKind::Bush: {
            DrawCircleV(center, obs.radius, Color{55, 125, 58, 255});
            DrawCircleV(Vector2{center.x - obs.radius * 0.4f, center.y}, obs.radius * 0.65f, Color{65, 140, 68, 255});
            DrawCircleV(Vector2{center.x + obs.radius * 0.4f, center.y - obs.radius * 0.2f}, obs.radius * 0.55f, Color{48, 112, 52, 255});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), obs.radius, Fade(BLACK, 0.4f));
            break;
        }
        case ObstacleKind::Cactus: {
            Color c = Color{60, 130, 70, 255};
            DrawRectangle(static_cast<int>(center.x - obs.radius * 0.22f), static_cast<int>(center.y - obs.radius),
                          static_cast<int>(obs.radius * 0.44f), static_cast<int>(obs.radius * 2.0f), c);
            DrawRectangle(static_cast<int>(center.x - obs.radius * 0.7f), static_cast<int>(center.y - obs.radius * 0.2f),
                          static_cast<int>(obs.radius * 0.4f), static_cast<int>(obs.radius * 0.9f), c);
            DrawRectangleLines(static_cast<int>(center.x - obs.radius * 0.22f), static_cast<int>(center.y - obs.radius),
                                static_cast<int>(obs.radius * 0.44f), static_cast<int>(obs.radius * 2.0f), Fade(BLACK, 0.4f));
            break;
        }
        case ObstacleKind::IceCrystal: {
            Color c = Color{190, 225, 240, 255};
            Vector2 top{center.x, center.y - obs.radius};
            Vector2 bottom{center.x, center.y + obs.radius * 0.6f};
            DrawTriangle(top, Vector2{center.x - obs.radius * 0.5f, center.y + obs.radius * 0.2f}, bottom, c);
            DrawTriangle(top, bottom, Vector2{center.x + obs.radius * 0.5f, center.y + obs.radius * 0.2f}, Color{160, 205, 225, 255});
            DrawTriangleLines(top, Vector2{center.x - obs.radius * 0.5f, center.y + obs.radius * 0.2f}, bottom, Fade(WHITE, 0.6f));
            break;
        }
        case ObstacleKind::Table: {
            Rectangle top{center.x - obs.radius, center.y - obs.radius * 0.6f, obs.radius * 2.0f, obs.radius * 1.2f};
            DrawRectangleRec(top, Color{178, 140, 90, 255});
            DrawRectangleLinesEx(top, 3.0f, Color{110, 82, 48, 255});
            DrawCircleV(Vector2{center.x - obs.radius * 0.4f, center.y}, obs.radius * 0.18f, Color{220, 220, 220, 255});
            DrawCircleV(Vector2{center.x + obs.radius * 0.4f, center.y}, obs.radius * 0.18f, Color{220, 220, 220, 255});
            break;
        }
        case ObstacleKind::Shelf: {
            Rectangle frame{center.x - obs.radius * 0.7f, center.y - obs.radius, obs.radius * 1.4f, obs.radius * 2.0f};
            DrawRectangleRec(frame, Color{120, 124, 132, 255});
            DrawRectangleLinesEx(frame, 3.0f, Color{70, 74, 82, 255});
            for (int i = 1; i <= 2; ++i) {
                float y = frame.y + frame.height * (static_cast<float>(i) / 3.0f);
                DrawLineEx(Vector2{frame.x, y}, Vector2{frame.x + frame.width, y}, 2.0f, Color{70, 74, 82, 255});
            }
            break;
        }
        case ObstacleKind::Counter: {
            Rectangle top{center.x - obs.radius, center.y - obs.radius * 0.55f, obs.radius * 2.0f, obs.radius * 1.1f};
            DrawRectangleRec(top, Color{190, 195, 200, 255});
            DrawRectangleLinesEx(top, 3.0f, Color{130, 135, 140, 255});
            DrawLineEx(Vector2{top.x, top.y}, Vector2{top.x + top.width, top.y}, 2.0f, Fade(WHITE, 0.6f));
            break;
        }
        case ObstacleKind::MeatRack: {
            DrawLineEx(Vector2{center.x - obs.radius, center.y - obs.radius}, Vector2{center.x + obs.radius, center.y - obs.radius},
                       4.0f, Color{80, 80, 85, 255});
            for (float dx = -obs.radius * 0.6f; dx <= obs.radius * 0.6f + 1.0f; dx += obs.radius * 0.6f) {
                Vector2 top{center.x + dx, center.y - obs.radius};
                Vector2 bottom{center.x + dx, center.y + obs.radius * 0.7f};
                DrawLineEx(top, bottom, 2.0f, Color{90, 90, 95, 255});
                DrawCircleV(Vector2{bottom.x, bottom.y - obs.radius * 0.3f}, obs.radius * 0.4f, Color{150, 60, 60, 255});
            }
            break;
        }
        case ObstacleKind::Rock:
        default: {
            DrawCircleV(center, obs.radius, Color{95, 98, 105, 255});
            DrawCircleV(Vector2{center.x - obs.radius * 0.4f, center.y - obs.radius * 0.3f}, obs.radius * 0.6f, Color{110, 113, 120, 255});
            DrawCircleV(Vector2{center.x + obs.radius * 0.35f, center.y + obs.radius * 0.3f}, obs.radius * 0.5f, Color{80, 83, 90, 255});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), obs.radius, Fade(BLACK, 0.5f));
            break;
        }
    }
}

}  // namespace fx
