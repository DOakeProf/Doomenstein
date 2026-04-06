#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/ActorHandle.hpp"

#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Renderer/Renderer.hpp"

Actor::Actor(Map* map, Vec3 const& position, float height, float radius, EulerAngles const& orientation /*= EulerAngles()*/, Rgba8 const& color /*= Rgba8::WHITE*/)
	: m_map(map)
	, m_position(position)
	, m_physicsHeight(height)
	, m_physicsRadius(radius)
	, m_orientation(orientation)
	, m_color(color)
{
	m_deathTimer = new Timer(1.f, m_map->m_game->m_gameClock);
}

Actor::~Actor()
{
	delete m_deathTimer;
	m_deathTimer = nullptr;

	delete m_handle;
	m_handle = nullptr;
}

void Actor::Update()
{
	if (m_health <= 0)
	{
		Die();
	}
	if (m_deathTimer->HasPeriodElapsed())
	{
		m_isGarbage = true;
	}
}

void Actor::Render() const
{
	g_engine->m_render->SetModelConstants(GetModelMatrix());
	std::vector<Vertex> m_verts;
	AddVertsForCylinder3D(m_verts, Vec3(), Vec3(0.f, 0.f, m_physicsHeight), m_physicsRadius, m_color, AABB2::ZERO_TO_ONE, 16);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->DrawVertexList(&m_verts);

	m_verts.clear();
	AddVertsForCylinder3D(m_verts, Vec3(), Vec3(0.f, 0.f, m_physicsHeight + 0.001f), m_physicsRadius + 0.001f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16);

	g_engine->m_render->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	g_engine->m_render->DrawVertexList(&m_verts);
}

Mat44 Actor::GetModelMatrix() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	Mat44 orientationMatrix = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	modelToWorld.Append(orientationMatrix);

	return modelToWorld;
}

void Actor::SetActorHandle(ActorHandle* handle)
{
	m_handle = handle;
}

void Actor::Die()
{
	m_isDead = true;
	m_deathTimer->Start();
}

void Actor::setStatic(bool status)
{
	m_isStatic = status;
}

