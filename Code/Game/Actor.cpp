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
#include "Engine/Math/RandomNumberGenerator.hpp"

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
	else
	{
		m_color = Rgba8::BLUE;
	}

	// Populate initial inventory
	for (int weaponIndex = 0; weaponIndex < m_definition->m_inventory.size(); ++weaponIndex)
	{
		m_weapons.push_back(new Weapon(m_map, m_definition->m_inventory[weaponIndex]));
	}
	if (m_weapons.size() > 0)
	{
		m_equippedWeapon = m_weapons[0];
	}

	m_health = m_definition->m_health;

	m_deathTimer = new Timer(m_definition->m_corpseLifetime, m_map->m_game->m_gameClock);
}

Actor::~Actor()
{
	delete m_deathTimer;
	m_deathTimer = nullptr;

	for (int weaponIndex = 0; weaponIndex < m_weapons.size(); ++weaponIndex)
	{
		delete m_weapons[weaponIndex];
		m_weapons[weaponIndex] = nullptr;
	}
}

void Actor::Update()
{
	Update_Physics();

	Update_Gameplay();

	m_isGrounded = false;
}

void Actor::Render() const
{
	Player* currentlyRenderedPlayer = m_map->GetCurrentRenderedPlayer();
	if (m_controller != nullptr && m_controller->IsPlayer() && currentlyRenderedPlayer->m_desiredPlayerState == PlayerState::FIRSTPERSON)
	{
		return;
	}

	g_engine->m_render->SetModelConstants(GetModelMatrixOnlyYaw());
	std::vector<Vertex> m_verts;

	Rgba8 colorToUse = m_color;
	if (m_isDead)
	{
		colorToUse.ScaleColor(0.5f);
	}

	AddVertsForCylinder3D(m_verts, Vec3(), Vec3(0.f, 0.f, m_definition->m_height), m_definition->m_radius, colorToUse, AABB2::ZERO_TO_ONE, 16);
	Vec3 displacementFromCenter = Vec3(m_definition->m_radius, 0.f, 0.f);
	displacementFromCenter.GetRotatedAboutZDegrees(m_orientation.m_yawDegrees);

	if (!m_definition->m_isFlying)
	{
		Vec3 noseStart = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter;
		Vec3 noseEnd = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter + displacementFromCenter * 0.3f;
		AddVertsForCone3D(m_verts, noseStart, noseEnd, 0.1f, colorToUse, AABB2::ZERO_TO_ONE, 16);
	}

	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->DrawVertexList(&m_verts);

	m_verts.clear();
	AddVertsForCylinder3D(m_verts, Vec3(), Vec3(0.f, 0.f, m_definition->m_height + 0.001f), m_definition->m_radius + 0.001f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16);

	if (!m_definition->m_isFlying)
	{
		Vec3 noseStart = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter;
		Vec3 noseEnd = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter + displacementFromCenter * 0.3f;
		AddVertsForCone3D(m_verts, noseStart, noseEnd, 0.1f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16);
	}

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

Mat44 Actor::GetModelMatrixOnlyYaw() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	EulerAngles orientationOnlyYaw = EulerAngles(m_orientation.m_yawDegrees, 0.f, 0.f);
	Mat44 orientationMatrix = orientationOnlyYaw.GetAsMatrix_IFwd_JLeft_KUp();
	modelToWorld.Append(orientationMatrix);

	return modelToWorld;
}

void Actor::Update_Physics()
{
	if (m_definition->m_physicsIsSimulated && !m_isDead)
	{
		float deltaSeconds = m_map->m_game->m_gameClock->GetDeltaSeconds();
		if (!m_definition->m_isFlying)
		{
			Vec3 gravityForce = Vec3(0.f, 0.f, -9.81f * 1.1f);
			float gravityMultiplier = 1.f + abs(GetClamped(m_velocity.z, -7.f, 0.f));
			if (!m_isGrounded)
			{
				gravityForce *= gravityMultiplier;
			}

			m_acceleration += gravityForce;
		}

		Vec3 dragForce = m_definition->m_drag * m_velocity * -1 * deltaSeconds;
		m_velocity += dragForce;

		m_velocity += m_acceleration * deltaSeconds;
		m_desiredPosition = m_position + m_velocity * deltaSeconds;

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

void Actor::Update_Gameplay()
{
	if (m_controller != nullptr && !m_controller->IsPlayer())
	{
		m_controller->Update();
	}

	if (m_health <= 0 && m_definition->m_health != -1)
	{
		Die();
	}

	if (m_isGrounded)
	{
		m_coyoteTime = 0.f;
	}
	else
	{
		m_coyoteTime += m_map->m_game->m_gameClock->GetDeltaSeconds();
	}

	if (m_deathTimer->HasPeriodElapsed())
	{
		m_isGarbage = true;
	}
}

void Actor::Update_Position()
{
	m_position = m_desiredPosition;
}

void Actor::SetActorHandle(ActorHandle* handle)
{
	m_handle = handle;
}

void Actor::MoveInDirection(Vec3 const& direction, float speed)
{
	if (!m_isDead)
	{
		float forceAmount = speed * m_definition->m_drag;
		AddForce(forceAmount * direction);
	}
}

void Actor::TurnInDirection(float angleToTurnTowards, float maximumTurn)
{
	if (!m_isDead)
	{
		m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, angleToTurnTowards, maximumTurn);
	}
}

void Actor::Jump(float jumpStrength)
{
	if (!m_isDead)
	{
		m_velocity.z = 0.f;
		AddImpulse(Vec3(0.f, 0.f, jumpStrength));
		m_isJumping = true;
		m_coyoteTime += 0.09f;
		m_isGrounded = false;
	}
}

void Actor::CancelJump()
{
	if (!m_isDead)
	{
		m_velocity.z *= 0.55f;
		m_isJumping = false;
	}
}

void Actor::Attack()
{
	if (!m_isDead && m_equippedWeapon != nullptr)
	{
		m_equippedWeapon->Fire(this);
	}
}

void Actor::SecondaryAttack()
{
	if (!m_isDead && m_equippedWeapon != nullptr)
	{
		m_equippedWeapon->AlternateFire(this);
	}
}

void Actor::EquipWeapon(Weapon* weapon)
{
	m_equippedWeapon = weapon;
}

void Actor::Damage(int damage, ActorHandle* otherActor)
{
	m_health -= damage;
	if (m_AIController != nullptr)
	{
		m_AIController->DamagedBy(otherActor);
	}
}

void Actor::Die()
{
	if (!m_isDead)
	{
		m_isDead = true;
		m_deathTimer->Start();
	}
}

void Actor::OnCollide(Actor* otherActor)
{
	if (otherActor != nullptr && m_definition->m_damageOnCollide != FloatRange(-1, -1))
	{
		float damage = m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_damageOnCollide.m_min, m_definition->m_damageOnCollide.m_max);
		otherActor->Damage(damage, m_owner->m_handle);
	}
	if (otherActor != nullptr && m_definition->m_impulseOnCollide != -1.f)
	{
		Vec3 vectorFromSelfToOther = (otherActor->m_position - m_position).GetNormalized();
		otherActor->AddImpulse(vectorFromSelfToOther * m_definition->m_impulseOnCollide);
	}
	if (m_definition->m_dieOnCollide)
	{
		Die();
	}
}

void Actor::OnPossessed()
{

}

void Actor::OnUnpossessed()
{
	if (m_AIController != nullptr)
	{
		m_controller = m_AIController;
	}
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
			newActorDef->m_collidesWithWorld = ParseXmlAttribute(*collisionElement, "collidesWithWorld", false);
			newActorDef->m_collidesWithActors = ParseXmlAttribute(*collisionElement, "collidesWithActors", false);
			newActorDef->m_damageOnCollide = ParseXmlAttribute(*collisionElement, "damageOnCollide", FloatRange(-1.f, -1.f));
			newActorDef->m_impulseOnCollide = ParseXmlAttribute(*collisionElement, "impulseOnCollide", -1.f);
			newActorDef->m_dieOnCollide = ParseXmlAttribute(*collisionElement, "dieOnCollide", false);
			newActorDef->m_collidesWithSameActor = ParseXmlAttribute(*collisionElement, "collidesWithSameActor", true);
		}

		XmlElement* physicsElement = actorDefElement->FirstChildElement("Physics");
		if (physicsElement != nullptr)
		{
			newActorDef->m_physicsIsSimulated = ParseXmlAttribute(*physicsElement, "simulated", false);
			newActorDef->m_walkSpeed = ParseXmlAttribute(*physicsElement, "walkSpeed", -1.f);
			newActorDef->m_runSpeed = ParseXmlAttribute(*physicsElement, "runSpeed", -1.f);
			newActorDef->m_turnSpeed = ParseXmlAttribute(*physicsElement, "turnSpeed", -1.f);
			newActorDef->m_drag = ParseXmlAttribute(*physicsElement, "drag", -1.f);
			newActorDef->m_isFlying = ParseXmlAttribute(*physicsElement, "flying", false);
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

		XmlElement* InventoryElement = actorDefElement->FirstChildElement("Inventory");
		if (InventoryElement != nullptr)
		{
			XmlElement* WeaponElement = InventoryElement->FirstChildElement();

			while (WeaponElement)
			{
				newActorDef->m_inventory.push_back(ParseXmlAttribute(*WeaponElement, "name", ""));
				WeaponElement = WeaponElement->NextSiblingElement();
			}
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
