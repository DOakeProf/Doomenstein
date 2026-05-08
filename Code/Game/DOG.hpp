#pragma once

#include "Engine/Math/Splines.hpp"

#include <vector>

class Actor;
class Map;
class Texture;

class DOG
{
public:
	DOG(Map* map);
	~DOG();

	void Update();
	void Render() const;
	void Update_MoveAlongSpline();

	void ChooseInitialSpline();
	void ChooseNextSpline();

	Vec3 FindRandomVec3();

private:
	Map* m_map;
	Texture* m_bodyTexture;
	Texture* m_tailTexture;

	Actor* m_head;
	Actor* m_tail;
	std::vector<Actor*> m_segments;
	int m_numSegments = 50;
	float m_followDistance = 2.5f;

	// Movement
	CubicHermiteSpline3D m_spline;
	float m_parametricValueAcrossCurve;
	float m_secondsUntilHit = 1.f;
	float m_averageVelocity = 20.f;
};