#include "Game/Actor.hpp"

#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Core/Engine.hpp"
#include "ENgine/Renderer/Renderer.hpp"

Actor::Actor(Vec3 const& position, float height, float radius, EulerAngles const& orientation /*= EulerAngles()*/, Rgba8 const& color /*= Rgba8::WHITE*/)
	: m_position(position)
	, m_physicsHeight(height)
	, m_physicsRadius(radius)
	, m_orientation(orientation)
	, m_color(color)
{

}

Actor::~Actor()
{

}

void Actor::Update()
{

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

void Actor::setStatic(bool status)
{
	m_isStatic = status;
}

