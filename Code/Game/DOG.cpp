#include "Game/DOG.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/SpawnInfo.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/Rift.hpp"

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/Timer.hpp"

DOG::DOG(Map* map)
	: m_map(map)
{
	SpawnInfo headSpawnInfo = SpawnInfo("DevourerHead", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	m_head = m_map->SpawnActor(headSpawnInfo);

	SpawnInfo bodySpawnInfo = SpawnInfo("DevourerBody", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	for (int bodyIndex = 0; bodyIndex < m_numSegments; ++bodyIndex)
	{
		Actor* bodySegment = m_map->SpawnActor(bodySpawnInfo);
		m_segments.push_back(bodySegment);
		bodySegment->m_shouldRouteDamageToOtherActor = true;
		bodySegment->m_actorToRouteDamageTo = m_head;
		if (bodyIndex != 0)
		{
			bodySegment->m_prevDOGSegment = m_segments[bodyIndex - 1];
		}
	}

	SpawnInfo tailSpawnInfo = SpawnInfo("DevourerBody", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	m_tail = m_map->SpawnActor(tailSpawnInfo);

	ChooseInitialSpline();

	m_bodyTexture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/DOGBody.png");

	m_movementBounds = AABB3(Vec3(-40.f, -40.f, -5.f), Vec3(120.f, 120.f, 70.f));

	m_riftGoAwayTimer = new Timer(10.f, m_map->m_game->m_gameClock);
	m_riftSpawnTimer = new Timer(15.f, m_map->m_game->m_gameClock);
	m_riftSpawnTimer->Start();
}

DOG::~DOG()
{

}

void DOG::Update()
{
	Update_MoveAlongSpline();

	if (m_head->m_isDead)
	{
		m_isDead = true;
		m_isGarbage = true;
	}

	if (m_DOGRift != nullptr && m_riftGoAwayTimer->HasPeriodElapsed() && m_DOGRift->m_actorsNearRift.size() == 0)
	{
		m_map->m_game->RemoveRift(m_DOGRift);
		m_riftGoAwayTimer->Stop();
	}

	if (m_riftSpawnTimer->DecrementPeriodIfElapsed())
	{
		m_riftGoAwayTimer->Start();

		float deltaSeconds = (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
		m_parametricValueAcrossCurve += 1.f / m_secondsUntilHit * deltaSeconds;
		Vec3 riftPosition = m_spline.EvaluateAtParametric(m_parametricValueAcrossCurve + 0.5f);
		Vec3 riftFwd = (m_spline.m_points[m_numPrevSplinePoints + 1] - m_spline.m_points[m_numPrevSplinePoints]).GetNormalized();
		Mat44 riftOrientationAsMatrix = Mat44(riftFwd, Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(0.f, 0.f, 0.f));
		EulerAngles riftOrientation = EulerAngles(riftOrientationAsMatrix);
		m_DOGRift = m_map->m_game->SpawnRift(riftPosition, riftOrientation, 5.f);

		m_riftSpawnTimer->m_period = m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(5.f,17.f);
		m_riftSpawnTimer->Start();
	}

	// Have body follow head.
	//Vec3 headToSegment = m_segments[0]->m_position - m_head->m_position;
	//if (headToSegment.GetLength() > m_followDistance)
	//{
	//	m_segments[0]->m_position = m_head->m_position + headToSegment.GetNormalized() * m_followDistance;
	//}
	//for (int bodyIndex = 1; bodyIndex < m_segments.size(); ++bodyIndex)
	//{
	//	Actor* curSegment = m_segments[bodyIndex];
	//	Actor* prevSegment = m_segments[bodyIndex - 1];
	//	Vec3 prevToCur =  curSegment->m_position - prevSegment->m_position;
	//	if (prevToCur.GetLength() > m_followDistance)
	//	{
	//		curSegment->m_position = prevSegment->m_position + prevToCur.GetNormalized() * m_followDistance;
	//	}
	//}
}

void DOG::Render(Map* currentlyRenderedMap) const
{
	
}

void DOG::Update_MoveAlongSpline()
{
	float deltaSeconds = (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
	m_parametricValueAcrossCurve += 1.f / m_secondsUntilHit * deltaSeconds;

	if (m_parametricValueAcrossCurve > (float)m_numPrevSplinePoints)
	{
		m_parametricValueAcrossCurve = (float)m_numPrevSplinePoints - 1.f;
		ChooseNextSpline();
	}

	m_head->m_desiredPosition = m_spline.EvaluateAtParametric(m_parametricValueAcrossCurve);

	for (int segmentIndex = 0; segmentIndex < m_segments.size(); ++segmentIndex)
	{
		m_segments[segmentIndex]->m_desiredPosition = m_spline.EvaluateAtParametricDisplacedByDistance(m_parametricValueAcrossCurve, -m_followDistance * (float)segmentIndex, 16);
	}

	if (g_app->IsDebug())
	{
		for (int prevIndex = 1; prevIndex < m_numPrevSplinePoints; ++prevIndex)
		{
			DebugAddWorldSphere(m_spline.m_points[prevIndex], 1.f, 0.f, Rgba8::MAGENTA);
		}
		DebugAddWorldSphere(m_spline.m_points[m_numPrevSplinePoints - 1], 1.f, 0.f, Rgba8::YELLOW);
		DebugAddWorldSphere(m_spline.m_points[m_numPrevSplinePoints], 1.f, 0.f, Rgba8::GREEN);
		DebugAddWorldSphere(m_spline.m_points[m_numPrevSplinePoints + 1], 1.f, 0.f, Rgba8::RED);
		DebugAddWorldSphere(m_spline.m_points[m_numPrevSplinePoints + 2], 1.f, 0.f, Rgba8::BLACK);
	}
}

void DOG::ChooseInitialSpline()
{
	// fake prev pos, Initial pos, next pos, next next pos.
	std::vector<Vec3> points;
	for (int prevIndex = 0; prevIndex < m_numPrevSplinePoints; ++prevIndex)
	{
		points.push_back(FindRandomVec3());
	}
	points.push_back(m_head->m_position);
	points.push_back(FindRandomVec3());
	points.push_back(FindRandomVec3());
	m_spline = CubicHermiteSpline3D(points);

	CubicHermiteCurve3D hermiteCurve = CubicHermiteCurve3D(m_spline.m_points[m_numPrevSplinePoints - 1], m_spline.m_velocities[m_numPrevSplinePoints - 1], m_spline.m_points[m_numPrevSplinePoints], m_spline.m_velocities[m_numPrevSplinePoints]);
	float lengthOfCurve = hermiteCurve.GetApproximateLength(4);
	m_secondsUntilHit = lengthOfCurve / m_averageVelocity;
}

void DOG::ChooseNextSpline()
{
	std::vector<Vec3> points;
	for (int prevIndexReversed = m_numPrevSplinePoints - 1; prevIndexReversed > 0; --prevIndexReversed)
	{
		points.push_back(m_spline.m_points[m_numPrevSplinePoints - prevIndexReversed]);
	}
	points.push_back(m_spline.m_points[m_numPrevSplinePoints]); // Spline position we just hit
	points.push_back(m_spline.m_points[m_numPrevSplinePoints + 1]); // Next spline position
	points.push_back(m_spline.m_points[m_numPrevSplinePoints + 2]); // Next Next spline position
	points.push_back(FindRandomPointInFront()); // a new random point, must be here to calculate next spline position's velocity.
	m_spline = CubicHermiteSpline3D(points, m_spline.m_velocities[m_numPrevSplinePoints]); // Have initial velocity be the velocity of the point we just hit.
	for (Vec3& velocity : m_spline.m_velocities)
	{
		velocity *= 2.f;
	}

	CubicHermiteCurve3D hermiteCurve = CubicHermiteCurve3D(m_spline.m_points[m_numPrevSplinePoints - 1], m_spline.m_velocities[m_numPrevSplinePoints - 1], m_spline.m_points[m_numPrevSplinePoints], m_spline.m_velocities[m_numPrevSplinePoints]);
	float lengthOfCurve = hermiteCurve.GetApproximateLength(4);
	m_secondsUntilHit = lengthOfCurve / m_averageVelocity;
}

Vec3 DOG::FindRandomVec3()
{
	float horizontalDisplacement = 20.f;
	float verticalDisplacement = 12.f;

	Vec3 randomVec3 = Vec3(
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(GetClamped(m_head->m_position.x - horizontalDisplacement, m_movementBounds.m_mins.x, m_movementBounds.m_maxs.x), GetClamped(m_head->m_position.x + horizontalDisplacement, m_movementBounds.m_mins.x, m_movementBounds.m_maxs.x)),
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(GetClamped(m_head->m_position.y - horizontalDisplacement, m_movementBounds.m_mins.y, m_movementBounds.m_maxs.y), GetClamped(m_head->m_position.y + horizontalDisplacement, m_movementBounds.m_mins.y, m_movementBounds.m_maxs.y)),
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(GetClamped(m_head->m_position.z - verticalDisplacement, m_movementBounds.m_mins.x, m_movementBounds.m_maxs.z), GetClamped(m_head->m_position.z + verticalDisplacement, m_movementBounds.m_mins.z, m_movementBounds.m_maxs.z))
	);
	return randomVec3;
}

Vec3 DOG::FindRandomPointInFront()
{
	Vec3 fwdDir = (m_spline.m_points[m_numPrevSplinePoints + 2] - m_spline.m_points[m_numPrevSplinePoints + 1]);
	fwdDir.z = 0.f;
	fwdDir = fwdDir.GetNormalized();

	Vec3 randomDirection = m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(fwdDir, 45.f);
	Vec3 randomVec3 = randomDirection * m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(27.f, 40.f) + m_head->m_position;

	int maxRandomRolls = 10;
	int randomRollsCount = 0;
	while (!IsPointInsideAABB3D(randomVec3, m_movementBounds))
	{
		++randomRollsCount;
		if (randomRollsCount > maxRandomRolls)
		{
			return FindRandomVec3();
		}

		randomDirection = m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(fwdDir, 35.f);
		randomVec3 = randomDirection * m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(27.f, 40.f) + m_head->m_position;
	}

	return randomVec3;
}
