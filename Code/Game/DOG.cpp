#include "Game/DOG.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/SpawnInfo.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Renderer/Texture.hpp"

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

	SpawnInfo tailSpawnInfo = SpawnInfo("DevourerBody", Vec3(-5.f, -5.f, 0.f), EulerAngles());
	m_tail = m_map->SpawnActor(tailSpawnInfo);

	ChooseInitialSpline();

	m_bodyTexture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/DOGBody.png");
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

void DOG::Render() const
{
	std::vector<Vertex_PCUTBN> localVerts;
	std::vector<unsigned int> localIndexes;

	float devourerSize = 2.5f;

	Vec3 BL1 = Vec3(devourerSize, devourerSize, 0.f);
	Vec3 BR1 = Vec3(devourerSize, -devourerSize, 0.f);
	Vec3 TR1 = Vec3(0.f, -devourerSize, 0.f);
	Vec3 TL1 = Vec3(0.f, devourerSize, 0.f);

	Vec3 BL2 = Vec3(devourerSize, 0.f, devourerSize);
	Vec3 BR2 = Vec3(devourerSize, 0.f, -devourerSize);
	Vec3 TR2 = Vec3(0.f, 0.f, -devourerSize);
	Vec3 TL2 = Vec3(0.f, 0.f, devourerSize);

	g_engine->m_render->BindTexture(m_bodyTexture);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_engine->m_render->BindShader(m_head->m_definition->m_shader);
	for (int bodyIndex = 1; bodyIndex < m_segments.size(); ++bodyIndex)
	{
		localVerts.clear();
		localIndexes.clear();
		Actor* curSegment = m_segments[bodyIndex];
		Actor* prevSegment = m_segments[bodyIndex - 1];

		AddVertsForRoundedQuad3D(localVerts, localIndexes, BL1, BR1, TR1, TL1);
		AddVertsForRoundedQuad3D(localVerts, localIndexes, BL2, BR2, TR2, TL2);

		// Render from prev to cur
		Vec3 prevToCur = curSegment->m_position - prevSegment->m_position;
		Mat44 prevToCurDirection = Mat44();
		prevToCurDirection.AppendTranslation3D(prevSegment->m_position + Vec3(0.f, 0.f, prevSegment->m_definition->m_height * 0.5f));
		prevToCurDirection.SetIJK3D(prevToCur.GetNormalized(), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f));
		prevToCurDirection.Orthonormalize_XFwd_YLeft_ZUp();
		g_engine->m_render->SetModelConstants(prevToCurDirection);
		g_engine->m_render->DrawIndexedVertexList(&localVerts, &localIndexes);
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
	float maxVertical = 30.f;
	float maxHorizontal = 100.f;

	float horizontalDisplacement = 40.f;
	float verticalDisplacement = 15.f;

	Vec3 randomVec3 = Vec3(
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(GetClamped(m_head->m_position.x - horizontalDisplacement, -maxHorizontal, maxHorizontal), GetClamped(m_head->m_position.x + horizontalDisplacement, -maxHorizontal, maxHorizontal)),
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(GetClamped(m_head->m_position.y - horizontalDisplacement, -maxHorizontal, maxHorizontal), GetClamped(m_head->m_position.y + horizontalDisplacement, -maxHorizontal, maxHorizontal)),
		m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(GetClamped(m_head->m_position.z - verticalDisplacement, -maxVertical, maxVertical), GetClamped(m_head->m_position.z + verticalDisplacement, -maxVertical, maxVertical))
	);
	return randomVec3;
}
