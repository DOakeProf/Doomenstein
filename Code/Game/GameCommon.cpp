#include "Game/GameCommon.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Engine.hpp"

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 32;
	constexpr int NUM_TRIS = 2 * NUM_SIDES; // Side is a trapezoid
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex verts[NUM_VERTS];
	constexpr float DEGREES_PER_SIDE = 360.f / (float)NUM_SIDES;

	for ( int sideNum = 0; sideNum < NUM_SIDES; ++ sideNum)
	{
		// Compute angle-related terms
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		// Compute inner & outer positions
		Vec3 innerStartPos = Vec3(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos = Vec3(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 innerEndPos = Vec3(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);
		Vec3 outerEndPos = Vec3(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);

		// Trapezoid made of two triangles; ABC and DEF
		// A is inner end, B is inner start, C is outer start
		// D is inner end, E is outer start, F is outer end
		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;
	}

	g_engine->m_render->DrawVertexArray(NUM_VERTS, verts);
}

void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;
	// The distance vector between the start and end, and the normalized forward and left vectors of the start.
	Vec2 distance = end - start;
	Vec2 forward = distance / distance.GetLength();
	Vec2 left = forward.GetRotatedBy90Degrees();
	constexpr int NUM_VERTS = 6;
	Vertex verts[NUM_VERTS];

	Vec3 startLeft = Vec3(start.x, start.y, 0) - (halfThickness * Vec3(forward.x, forward.y, 0)) + (halfThickness * Vec3(left.x, left.y, 0));
	Vec3 startRight = Vec3(start.x, start.y, 0) - (halfThickness * Vec3(forward.x, forward.y, 0)) - (halfThickness * Vec3(left.x, left.y, 0));
	Vec3 endLeft = Vec3(end.x, end.y, 0) + (halfThickness * Vec3(forward.x, forward.y, 0)) + (halfThickness * Vec3(left.x, left.y, 0));
	Vec3 endRight = Vec3(end.x, end.y, 0) + (halfThickness * Vec3(forward.x, forward.y, 0)) - (halfThickness * Vec3(left.x, left.y, 0));

	// Line made of two triangles.
	// Index 0 is start left, 1 is start right, 2 is end right.
	// Index 3 is start left, 4 is end right, and 5 is end left.
	verts[0].m_position = startLeft;
	verts[1].m_position = startRight;
	verts[2].m_position = endRight;
	verts[0].m_color = color;
	verts[1].m_color = color;
	verts[2].m_color = color;

	verts[3].m_position = startLeft;
	verts[4].m_position = endRight;
	verts[5].m_position = endLeft;
	verts[3].m_color = color;
	verts[4].m_color = color;
	verts[5].m_color = color;

	g_engine->m_render->DrawVertexArray(NUM_VERTS, verts);
}
void DrawFadedRing(Vec2 center, float innerRadius, float outerRadius, Rgba8 innerColor, Rgba8 outerColor)
{
	constexpr int NUM_SIDES = 32;
	constexpr int NUM_TRIS = 2 * NUM_SIDES; // Side is a trapezoid
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex verts[NUM_VERTS];
	constexpr float DEGREES_PER_SIDE = 360.f / (float)NUM_SIDES;

	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		// Compute angle-related terms
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		// Compute inner & outer positions
		Vec3 innerStartPos = Vec3(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos = Vec3(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 innerEndPos = Vec3(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);
		Vec3 outerEndPos = Vec3(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);

		// Trapezoid made of two triangles; ABC and DEF
		// A is inner end, B is inner start, C is outer start
		// D is inner end, E is outer start, F is outer end
		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = innerColor;
		verts[vertIndexB].m_color = innerColor;
		verts[vertIndexC].m_color = outerColor;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = innerColor;
		verts[vertIndexE].m_color = outerColor;
		verts[vertIndexF].m_color = outerColor;
	}

	g_engine->m_render->DrawVertexArray(NUM_VERTS, verts);
}
void DrawGlow(Vec2 pos, Rgba8 color, float alphaValue, float radius)
{
	color.ScaleAlpha(alphaValue);

	float innerRadius = 0.f;
	float outerRadius = radius;
	Rgba8 innerColor = color;
	Rgba8 outerColor(innerColor.r, innerColor.g, innerColor.b, 0);
	DrawFadedRing(pos, innerRadius, outerRadius, innerColor, outerColor);
}