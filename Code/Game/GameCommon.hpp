#pragma once

struct Vec2;
struct Rgba8;

// constexpr means that its a compile time constant.
constexpr float WORLD_MIN_X = -1;
constexpr float WORLD_MAX_X = 1;
constexpr float WORLD_MIN_Y = -1;
constexpr float WORLD_MAX_Y = 1;

constexpr float WORLD_SIZE_X = WORLD_MAX_X - WORLD_MIN_X;
constexpr float WORLD_SIZE_Y = WORLD_MAX_Y - WORLD_MIN_Y;
constexpr float WORLD_CENTER_X = WORLD_SIZE_X / 2.f;
constexpr float WORLD_CENTER_Y = WORLD_SIZE_Y / 2.f;

constexpr float SCREEN_ASPECT = 2.f;
constexpr float SCREEN_SIZE_Y = 800.f;
constexpr float SCREEN_SIZE_X = SCREEN_SIZE_Y * SCREEN_ASPECT;
constexpr float SCREEN_CENTER_X = SCREEN_SIZE_X / 2.f;
constexpr float SCREEN_CENTER_Y = SCREEN_SIZE_Y / 2.f;


constexpr float COMMON_PI = 3.14159265358979323846f;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
void DrawFadedRing(Vec2 center, float innerRadius, float outerRadius, Rgba8 innerColor, Rgba8 outerColor);
void DrawGlow(Vec2 pos, Rgba8 color, float alphaValue, float radius);