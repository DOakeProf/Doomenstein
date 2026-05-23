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
	Actor* m_tail;
	std::vector<Actor*> m_segments;
private:
	Map* m_map;
	Texture* m_bodyTexture;
	Texture* m_tailTexture;

	Rift* m_DOGRift = nullptr;
	Timer* m_riftGoAwayTimer = nullptr;
	Timer* m_riftSpawnTimer = nullptr;
	Timer* m_playerChargeAtTimer = nullptr;
	Timer* m_dpsPhaseTimer = nullptr;

	int m_numSegments = 50;
	float m_followDistance = 4.f;

	Actor* m_ballInMouth = nullptr;

	// Movement
	CubicHermiteSpline3D m_spline;
	int m_numPrevSplinePoints = 20; // The amount of previous spline points to track, these are needed in order to properly place the body segments of the devourer.
	float m_parametricValueAcrossCurve = 1.f;
	float m_secondsUntilHit = 1.f;
	float m_averageVelocity = 35.f;
	Vec3 m_prevHeadPos = Vec3();
	bool m_shouldChargeAtSpecificPoint = false;
	Vec3 m_specificPointToChargeAt = Vec3();
	AABB3 m_movementBounds;
};