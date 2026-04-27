#include "Game/DOG.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/SpawnInfo.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

DOG::DOG(Map* map)
	: m_map(map)
{
	SpawnInfo headSpawnInfo = SpawnInfo("DevourerHead", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	m_head = m_map->SpawnActor(headSpawnInfo);

	SpawnInfo bodySpawnInfo = SpawnInfo("DevourerBody", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	for (int bodyIndex = 0; bodyIndex < m_numSegments; ++bodyIndex)
	{
		m_segments.push_back(m_map->SpawnActor(bodySpawnInfo));
	}

	ChooseInitialSpline();
}

DOG::~DOG()
{

}

void DOG::Update()
{
	Update_MoveAlongSpline();

	// Have body follow head.
	Vec3 headToSegment = m_segments[0]->m_position - m_head->m_position;
	if (headToSegment.GetLength() > m_followDistance)
	{
		m_segments[0]->m_position = m_head->m_position + headToSegment.GetNormalized() * m_followDistance;
	}
	for (int bodyIndex = 1; bodyIndex < m_segments.size(); ++bodyIndex)
	{
		Actor* curSegment = m_segments[bodyIndex];
		Actor* prevSegment = m_segments[bodyIndex - 1];
		Vec3 prevToCur =  curSegment->m_position - prevSegment->m_position;
		if (prevToCur.GetLength() > m_followDistance)
		{
			curSegment->m_position = prevSegment->m_position + prevToCur.GetNormalized() * m_followDistance;
		}
	}
}

void DOG::Update_MoveAlongSpline()
{
	float deltaSeconds = (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
	m_parametricValueAcrossCurve += 1 / m_secondsUntilHit * deltaSeconds;

	if (m_parametricValueAcrossCurve > 1.f)
	{
		m_parametricValueAcrossCurve = 0.f;
		ChooseNextSpline();
	}

	m_head->m_position = m_spline.EvaluateAtParametric(m_parametricValueAcrossCurve);

	if (g_app->IsDebug())
	{
		DebugAddWorldSphere(m_spline.m_points[0], 1.f, 0.f, Rgba8::GREEN);
		DebugAddWorldSphere(m_spline.m_points[1], 1.f, 0.f, Rgba8::GREEN);
		DebugAddWorldSphere(m_spline.m_points[2], 1.f, 0.f, Rgba8::GREEN);
	}
}

void DOG::ChooseInitialSpline()
{
	// Initial pos, next pos, next next pos.
	std::vector<Vec3> points;
	points.push_back(m_head->m_position);
	points.push_back(FindRandomVec3());
	points.push_back(FindRandomVec3());
	m_spline = CubicHermiteSpline3D(points);

	CubicHermiteCurve3D hermiteCurve = CubicHermiteCurve3D(m_spline.m_points[0], m_spline.m_velocities[0], m_spline.m_points[1], m_spline.m_velocities[1]);
	float lengthOfCurve = hermiteCurve.GetApproximateLength(4);
	m_secondsUntilHit = lengthOfCurve / m_averageVelocity;
}

void DOG::ChooseNextSpline()
{
	std::vector<Vec3> points;
	points.push_back(m_head->m_position);
	points.push_back(m_spline.m_points[2]);
	points.push_back(FindRandomVec3());
	m_spline = CubicHermiteSpline3D(points, m_spline.m_velocities[1]); // Have initial velocity be the velocity of the point we just hit.

	CubicHermiteCurve3D hermiteCurve = CubicHermiteCurve3D(m_spline.m_points[0], m_spline.m_velocities[0], m_spline.m_points[1], m_spline.m_velocities[1]);
	float lengthOfCurve = hermiteCurve.GetApproximateLength(4);
	m_secondsUntilHit = lengthOfCurve / m_averageVelocity;
}

Vec3 DOG::FindRandomVec3()
{
	Vec3 randomVec3 = Vec3(
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(-80.f, 100.f),
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(-80.f, 100.f),
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(-10.f, 20.f)
	);
	return randomVec3;
}
