#include "Game/DOG.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/SpawnInfo.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/Rift.hpp"
#include "Game/Player.hpp"

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
	SpawnInfo bodyNearHeadSpawnInfo = SpawnInfo("DevourerBody_NearHead", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	for (int bodyIndex = 0; bodyIndex < m_numSegments; ++bodyIndex)
	{
		Actor* bodySegment = nullptr;
		if (bodyIndex < 2)
		{
			bodySegment = m_map->SpawnActor(bodyNearHeadSpawnInfo);
		}
		else
		{
			bodySegment = m_map->SpawnActor(bodySpawnInfo);
		}
		m_segments.push_back(bodySegment);
		bodySegment->m_shouldRouteDamageToOtherActor = true;
		bodySegment->m_actorToRouteDamageTo = m_head;
		if (bodyIndex != 0)
		{
			bodySegment->m_prevDOGSegment = m_segments[bodyIndex - 1];
		}
	}

	SpawnInfo tailSpawnInfo = SpawnInfo("DevourerTail", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	m_tail = m_map->SpawnActor(tailSpawnInfo);
	m_tail->m_shouldRouteDamageToOtherActor = true;
	m_tail->m_actorToRouteDamageTo = m_head;
	m_tail->m_prevDOGSegment = m_segments[m_numSegments - 1];

	ChooseInitialSpline();

	m_bodyTexture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/DOGBody.png");
	m_tailTexture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/DOGTail.png");

	m_movementBounds = AABB3(Vec3(-40.f, -40.f, 10.f), Vec3(120.f, 120.f, 70.f));

	m_riftGoAwayTimer = new Timer(10.f, m_map->m_game->m_gameClock);
	m_riftSpawnTimer = new Timer(15.f, m_map->m_game->m_gameClock);
	m_playerChargeAtTimer = new Timer(20.f, m_map->m_game->m_gameClock);
	m_playerChargeAtTimer->Start();
	m_dpsPhaseTimer = new Timer(25.f, m_map->m_game->m_gameClock);
	m_riftSpawnTimer->Start();
}

DOG::~DOG()
{

}

void DOG::Update()
{
	Update_MoveAlongSpline();

	for (Player* player : m_map->GetPlayers())
	{
		if (player != nullptr && (m_playerChargeAtTimer->HasPeriodElapsed() || player->m_ballInsideOf != nullptr) && m_dpsPhaseTimer->IsStopped())
		{
			float randomNumber = m_map->m_game->m_randomNumberGenerator->RollRandomFloatZeroToOne();
			if (!m_shouldChargeAtSpecificPoint) // If we haven't started charging yet, do so at the first player we see.
			{
				m_specificPointToChargeAt = player->m_position;
				m_shouldChargeAtSpecificPoint = true;
				m_playerChargeAtTimer->Start();
			}
			else if (m_shouldChargeAtSpecificPoint && randomNumber < 0.5f) // If we already have a player 50 charge at, 50% chance we charge at this next one instead.
			{
				m_specificPointToChargeAt = player->m_position;
				m_shouldChargeAtSpecificPoint = true;
				m_playerChargeAtTimer->Start();
			}
		}
	}

	if (m_ballInMouth == nullptr)
	{
		for (Actor* actor : m_map->GetActors())
		{
			if (actor != nullptr && actor->m_definition->m_name == "Ball" &&
				actor->m_map == m_head->m_map && // Must be on same map
				DoCylindersZOverlap(
					Vec2(m_head->m_position.x, m_head->m_position.y), m_head->m_definition->m_radius, m_head->m_position.z, m_head->m_position.z + m_head->m_definition->m_height,
					Vec2(actor->m_position.x, actor->m_position.y), actor->m_definition->m_radius, actor->m_position.z, actor->m_position.z + actor->m_definition->m_height)
				)
			{
				m_ballInMouth = actor;
				m_dpsPhaseTimer->Start();
			}
		}
	}

	if (m_ballInMouth)
	{
		Vec3 newPosition = m_spline.EvaluateAtParametricDisplacedByDistance(m_parametricValueAcrossCurve, m_followDistance, 16);
		if (newPosition != Vec3())
		{
			m_ballInMouth->m_desiredPosition = Vec3(0.f, 0.f, -0.7f) + newPosition;
			Mat44 ballBillboardedToHead = GetBillboardTransform(BillboardType::FULL_FACING, m_head->GetModelMatrix(), m_ballInMouth->m_desiredPosition);
			ballBillboardedToHead.AppendZRotation(45.f);
			m_ballInMouth->m_orientation = EulerAngles(ballBillboardedToHead);
			
		}
		if (m_ballInMouth->m_map != m_head->m_map)
		{
			m_ballInMouth->m_hasEnteredRift = true;
		}
		for (Player* player : m_map->m_game->m_players)
		{
			if (player != nullptr && player->m_ballInsideOf != nullptr)
			{
				player->GetActor()->m_desiredPosition = player->m_ballInsideOf->m_desiredPosition;
				if (player->GetActor()->m_map != m_head->m_map)
				{
					player->GetActor()->m_hasEnteredRift = true;
				}
			}
		}
	}

	if (m_dpsPhaseTimer->DecrementPeriodIfElapsed())
	{
		m_dpsPhaseTimer->Stop();
		m_ballInMouth->Die();
		m_ballInMouth = nullptr;
	}

	if (m_head->m_isDead)
	{
		m_isDead = true;
		m_isGarbage = true;

		for (Actor* segment : m_segments)
		{
			segment->m_isDead = true;
			segment->m_isGarbage = true;
		}

		m_tail->m_isDead = true;
		m_tail->m_isGarbage = true;
	}

	if (m_DOGRift != nullptr && m_riftGoAwayTimer->HasPeriodElapsed() && m_DOGRift->m_actorsNearRift.size() == 0)
	{
		m_DOGRift->Die();
		m_DOGRift = nullptr;
		m_riftGoAwayTimer->Stop();
		m_riftSpawnTimer->Start();

		// Sometimes the DOG can get part of it caught in the other world, this just does a check and forces every part to be in the same world as the DOG when the rift closes.
		for (Actor* segment : m_segments)
		{
			if (segment->m_map != m_head->m_map)
			{
				segment->m_hasEnteredRift = true;
			}
		}
		if (m_tail->m_map != m_head->m_map)
		{
			m_tail->m_hasEnteredRift = true;
		}
	}

	// Spawn a rift along the spline
	if (m_riftSpawnTimer->DecrementPeriodIfElapsed())
	{
		m_riftGoAwayTimer->Start();

		float deltaSeconds = (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
		m_parametricValueAcrossCurve += 1.f / m_secondsUntilHit * deltaSeconds;
		float newParametricValue = m_parametricValueAcrossCurve + 0.5f;
		float newParametricRemainder = fmod(newParametricValue, 1.f);
		while (newParametricRemainder < 0.15f || newParametricRemainder > 0.85f)
		{
			newParametricValue += 0.1f;
			newParametricRemainder = fmod(newParametricValue, 1.f);
		}
		Vec3 riftPosition = m_spline.EvaluateAtParametric(newParametricValue);
		Vec3 riftPositionRightInFrontOf = m_spline.EvaluateAtParametric(newParametricValue + 0.01f);
		Vec3 riftFwd = (riftPositionRightInFrontOf - riftPosition).GetNormalized();
		Mat44 riftOrientationAsMatrix = Mat44(riftFwd, Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(0.f, 0.f, 0.f));
		EulerAngles riftOrientation = EulerAngles(riftOrientationAsMatrix);
		m_DOGRift = m_map->m_game->SpawnRift(riftPosition, riftOrientation, 4.f);

		m_riftSpawnTimer->m_period = m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(5.f,17.f);
		m_riftSpawnTimer->Stop();
	}
}

void DOG::Render([[maybe_unused]] Map* currentlyRenderedMap) const
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

	Vec3 newPosition = m_spline.EvaluateAtParametricDisplacedByDistance(m_parametricValueAcrossCurve, -m_followDistance * 0.45f, 8) - Vec3(0.f, 0.f, 2.f);
	if (newPosition != Vec3())
	{
		m_head->m_desiredPosition = newPosition;
	}

	for (int segmentIndex = 0; segmentIndex < m_segments.size(); ++segmentIndex)
	{
		newPosition = m_spline.EvaluateAtParametricDisplacedByDistance(m_parametricValueAcrossCurve, -m_followDistance * ((float)segmentIndex + 1.f), 8);
		if (newPosition != Vec3())
		{
			m_segments[segmentIndex]->m_desiredPosition = newPosition;
		}
	}	

	newPosition = m_spline.EvaluateAtParametricDisplacedByDistance(m_parametricValueAcrossCurve, -m_followDistance * ((float)m_segments.size() + 1.f), 8);
	if (newPosition != Vec3())
	{
		m_tail->m_desiredPosition = newPosition;
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
	for (int prevIndex = 0; prevIndex < m_numPrevSplinePoints - 1; ++prevIndex)
	{
		points.push_back(FindRandomVec3());
	}
	points.push_back(Vec3( -113.f, -46.f, 93.5f));
	points.push_back(Vec3(-75.6f, 19.f, 25.f));
	points.push_back(Vec3(-33.f, 31.f, 16.f));
	points.push_back(Vec3(0, 0.f, 5.f));
	m_spline = CubicHermiteSpline3D(points);

	CubicHermiteCurve3D hermiteCurve = CubicHermiteCurve3D(m_spline.m_points[m_numPrevSplinePoints - 1], m_spline.m_velocities[m_numPrevSplinePoints - 1], m_spline.m_points[m_numPrevSplinePoints], m_spline.m_velocities[m_numPrevSplinePoints]);
	float lengthOfCurve = hermiteCurve.GetApproximateLength(4);
	m_secondsUntilHit = lengthOfCurve / m_averageVelocity;
	m_parametricValueAcrossCurve = (float)m_numPrevSplinePoints - 1.f;
}

void DOG::ChooseNextSpline()
{
	std::vector<Vec3> points;
	for (int prevIndexReversed = m_numPrevSplinePoints - 1; prevIndexReversed > 0; --prevIndexReversed)
	{
		points.push_back(m_spline.m_points[m_numPrevSplinePoints - prevIndexReversed]);
	}
	points.push_back(m_spline.m_points[m_numPrevSplinePoints]); // Spline position we just hit

	if (m_shouldChargeAtSpecificPoint)
	{
		points.push_back(m_specificPointToChargeAt); // Next spline position
		points.push_back(FindRandomVec3()); // Next Next spline position
		points.push_back(FindRandomVec3()); // a new random point, must be here to calculate next spline position's velocity.
		m_shouldChargeAtSpecificPoint = false;
	}
	else
	{
		points.push_back(m_spline.m_points[m_numPrevSplinePoints + 1]); // Next spline position
		points.push_back(m_spline.m_points[m_numPrevSplinePoints + 2]); // Next Next spline position
		points.push_back(FindRandomVec3()); // a new random point, must be here to calculate next spline position's velocity.
	}
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
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(GetClamped(m_head->m_position.z - verticalDisplacement, m_movementBounds.m_mins.z, m_movementBounds.m_maxs.z), GetClamped(m_head->m_position.z + verticalDisplacement, m_movementBounds.m_mins.z, m_movementBounds.m_maxs.z))
	);
	return randomVec3;
}

Vec3 DOG::FindRandomPointInFront()
{
	Vec3 fwdDir = (m_spline.m_points[m_numPrevSplinePoints + 2] - m_spline.m_points[m_numPrevSplinePoints + 1]);
	fwdDir.z = 0.f;
	fwdDir = fwdDir.GetNormalized();

	Vec3 randomDirection = m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(fwdDir, 90.f);
	Vec3 randomVec3 = randomDirection * m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(5.f, 30.f) + m_head->m_position;

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
		randomVec3 = randomDirection * m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(17.f, 30.f) + m_head->m_position;
	}

	return randomVec3;
}
