#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/AI.hpp"
#include "Game/Player.hpp"

#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"

std::vector<ActorDefinition*> ActorDefinition::s_definitions;

Actor::Actor(Map* map, std::string name, Vec3 const& position, EulerAngles const& orientation /*= EulerAngles()*/)
	: m_map(map)
	, m_position(position)
	, m_orientation(orientation)
{
	m_definition = ActorDefinition::GetByName(name);

	if (m_definition->m_name == "Marine")
	{
		m_color = Rgba8::GREEN;
	}
	else if (m_definition->m_name == "Demon")
	{
		m_color = Rgba8::RED;
	}

	m_health = m_definition->m_health;

	m_deathTimer = new Timer(m_definition->m_corpseLifetime, m_map->m_game->m_gameClock);
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
	Update_Physics();

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
	AddVertsForCylinder3D(m_verts, Vec3(), Vec3(0.f, 0.f, m_definition->m_height), m_definition->m_radius, m_color, AABB2::ZERO_TO_ONE, 16);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->DrawVertexList(&m_verts);

	m_verts.clear();
	AddVertsForCylinder3D(m_verts, Vec3(), Vec3(0.f, 0.f, m_definition->m_height + 0.001f), m_definition->m_radius + 0.001f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16);

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

void Actor::Update_Physics()
{
	if (m_definition->m_physicsIsSimulated)
	{
		float deltaSeconds = m_map->m_game->m_gameClock->GetDeltaSeconds();
		/*if (!m_definition->m_isFlying)
		{
			m_velocity.z = 0.f;
		}*/
		Vec3 gravityForce = Vec3(0.f, 0.f, -9.81f);
		float gravityMultiplier = 1.f + abs(GetClamped(m_velocity.z, -10.f, 0.f));
		gravityForce *= gravityMultiplier;

		m_acceleration += gravityForce;

		Vec3 dragForce = m_definition->m_drag * m_velocity * -1 * deltaSeconds;
		m_velocity += dragForce;

		m_velocity += m_acceleration * deltaSeconds;
		m_position += m_velocity * deltaSeconds;

		m_acceleration = Vec3();
	}
}

void Actor::AddForce(Vec3 const& force)
{
	m_acceleration += force;
}

void Actor::AddImpulse(Vec3 const& impulse)
{
	m_velocity += impulse;
}

void Actor::SetActorHandle(ActorHandle* handle)
{
	m_handle = handle;
}

void Actor::MoveInDirection(Vec3 const& direction, float speed)
{
	float forceAmount = speed * m_definition->m_drag;
	AddForce(forceAmount * direction);
}

void Actor::TurnInDirection(float angleToTurnTowards, float maximumTurn)
{
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, angleToTurnTowards, maximumTurn);
}

void Actor::Damage(int damage)
{
	m_health -= damage;
	// Notify controller of source?
}

void Actor::Die()
{
	m_isDead = true;
	m_deathTimer->Start();
}

void ActorDefinition::InitializeDefinitions(const char* path)
{
	XmlDocument tileDefsXml;
	[[maybe_unused]] XmlResult result = tileDefsXml.LoadFile(path);
	XmlElement* rootElement = tileDefsXml.RootElement();
	XmlElement* actorDefElement = rootElement->FirstChildElement();

	while (actorDefElement)
	{
		ActorDefinition* newActorDef = new ActorDefinition();
		newActorDef->m_name = ParseXmlAttribute(*actorDefElement, "name", "");
		newActorDef->m_faction = ParseXmlAttribute(*actorDefElement, "faction", "");
		newActorDef->m_health = ParseXmlAttribute(*actorDefElement, "health", -1);
		newActorDef->m_canBePossessed = ParseXmlAttribute(*actorDefElement, "canBePossessed", false);
		newActorDef->m_corpseLifetime = ParseXmlAttribute(*actorDefElement, "corpseLifetime", -1.f);
		newActorDef->m_visible = ParseXmlAttribute(*actorDefElement, "visible", true);

		XmlElement* collisionElement = actorDefElement->FirstChildElement("Collision");
		if (collisionElement != nullptr)
		{
			newActorDef->m_radius = ParseXmlAttribute(*collisionElement, "radius", -1.f);
			newActorDef->m_height = ParseXmlAttribute(*collisionElement, "height", -1.f);
			newActorDef->m_collidesWithWorld = ParseXmlAttribute(*collisionElement, "collidesWithWorld", true);
			newActorDef->m_collidesWithActors = ParseXmlAttribute(*collisionElement, "collidesWithActors", true);
		}

		XmlElement* physicsElement = actorDefElement->FirstChildElement("Physics");
		if (physicsElement != nullptr)
		{
			newActorDef->m_physicsIsSimulated = ParseXmlAttribute(*physicsElement, "simulated", false);
			newActorDef->m_walkSpeed = ParseXmlAttribute(*physicsElement, "walkSpeed", -1.f);
			newActorDef->m_runSpeed = ParseXmlAttribute(*physicsElement, "runSpeed", -1.f);
			newActorDef->m_turnSpeed = ParseXmlAttribute(*physicsElement, "turnSpeed", -1.f);
			newActorDef->m_drag = ParseXmlAttribute(*physicsElement, "drag", -1.f);
		}

		XmlElement* cameraElement = actorDefElement->FirstChildElement("Camera");
		if (cameraElement != nullptr)
		{
			newActorDef->m_eyeHeight = ParseXmlAttribute(*cameraElement, "eyeHeight", -1.f);
			newActorDef->m_cameraFOV = ParseXmlAttribute(*cameraElement, "cameraFOV", -1.f);
		}

		XmlElement* AIElement = actorDefElement->FirstChildElement("AI");
		if (AIElement != nullptr)
		{
			newActorDef->m_aiEnabled = ParseXmlAttribute(*AIElement, "aiEnabled", false);
			newActorDef->m_sightRadius = ParseXmlAttribute(*AIElement, "sightRadius", -1.f);
			newActorDef->m_sightAngle = ParseXmlAttribute(*AIElement, "sightAngle", -1.f);
		}

		s_definitions.push_back(newActorDef);
		actorDefElement = actorDefElement->NextSiblingElement();
	}
}

void ActorDefinition::ClearDefinitions()
{
	s_definitions.clear();
}

const ActorDefinition* ActorDefinition::GetByName(const std::string& name)
{
	for (int mapIndex = 0; mapIndex < s_definitions.size(); ++mapIndex)
	{
		ActorDefinition* currentDef = s_definitions[mapIndex];
		if (currentDef->m_name == name)
		{
			return currentDef;
		}
	}
	return nullptr;
}
