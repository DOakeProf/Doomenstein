#pragma once

#include "Engine/Math/Splines.hpp"
#include "Engine/Math/AABB3.hpp"

#include <vector>

class Actor;
class Map;
class Texture;
class Rift;
class Timer;

class DOG
{
public:
	DOG(Map* map);
	~DOG();

	void Update();
	void Render(Map* currentlyRenderedMap) const;
	void Update_MoveAlongSpline();

	void ChooseInitialSpline();
	void ChooseNextSpline();

	Vec3 FindRandomVec3();
	Vec3 FindRandomPointInFront();

	Actor* m_head;
	bool m_isDead;
	bool m_isGarbage;
private:
	Map* m_map;
	Texture* m_bodyTexture;
	Texture* m_tailTexture;

	Rift* m_DOGRift = nullptr;
	Timer* m_riftGoAwayTimer = nullptr;
	Timer* m_riftSpawnTimer = nullptr;

	Actor* m_tail;
	std::vector<Actor*> m_segments;
	int m_numSegments = 50;
	float m_followDistance = 2.5f;

	// Movement
	CubicHermiteSpline3D m_spline;
	int m_numPrevSplinePoints = 10; // The amount of previous spline points to track, these are needed in order to properly place the body segments of the devourer.
	float m_parametricValueAcrossCurve = 1.f;
	float m_secondsUntilHit = 1.f;
	float m_averageVelocity = 25.f;
	Vec3 m_prevHeadPos = Vec3();
	AABB3 m_movementBounds;
};