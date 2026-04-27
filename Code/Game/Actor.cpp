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
#include "Engine/Core/ErrorWarningAssert.hpp"

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

	// Add verts according to visual information.
	Vec2 pivot = m_definition->m_pivot * m_definition->m_size;
	Vec2 halfSize = m_definition->m_size * 0.5f;
	float eyeHeight = m_definition->m_eyeHeight;
	if (m_definition->m_renderRounded)
	{
		//AddVertsForQuad3D(
		//	m_verts,
		//	m_vertexIndexes,
		//	Vec3(0.f, -pivot.x + 0.f, -pivot.y + 0.f),
		//	Vec3(0.f, -pivot.x + halfSize.x, -pivot.y + 0.f),
		//	Vec3(0.f, -pivot.x + halfSize.x, -pivot.y + m_definition->m_size.y),
		//	Vec3(0.f, -pivot.x + 0.f, -pivot.y + m_definition->m_size.y)
		//);
		//AddVertsForQuad3D(
		//	m_verts,
		//	m_vertexIndexes,
		//	Vec3(0.f, -pivot.x + halfSize.x, -pivot.y + 0.f),
		//	Vec3(0.f, -pivot.x + m_definition->m_size.x, -pivot.y + 0.f),
		//	Vec3(0.f, -pivot.x + m_definition->m_size.x, -pivot.y + m_definition->m_size.y),
		//	Vec3(0.f, -pivot.x + halfSize.x, -pivot.y + m_definition->m_size.y)
		//);
		AddVertsForRoundedQuad3D(
			m_verts,
			m_vertexIndexes,
			Vec3(0.f, -pivot.x + 0.f, -pivot.y + 0.f),
			Vec3(0.f, -pivot.x + m_definition->m_size.x, -pivot.y + 0.f),
			Vec3(0.f, -pivot.x + m_definition->m_size.x, -pivot.y + m_definition->m_size.y),
			Vec3(0.f, -pivot.x + 0.f, -pivot.y + m_definition->m_size.y)
		);
	}
	else
	{
		AddVertsForQuad3D(
			m_verts,
			m_vertexIndexes,
			Vec3(0.f, -pivot.x + 0.f, -pivot.y + 0.f),
			Vec3(0.f, -pivot.x + m_definition->m_size.x, -pivot.y + 0.f),
			Vec3(0.f, -pivot.x + m_definition->m_size.x, -pivot.y + m_definition->m_size.y),
			Vec3(0.f, -pivot.x + 0.f, -pivot.y + m_definition->m_size.y)
		);
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

	m_animClock = new Clock(*m_map->m_game->m_gameClock);
	m_deathTimer = new Timer(m_definition->m_corpseLifetime, m_map->m_game->m_gameClock);
	m_animTimer = new Timer(0.f, m_animClock);

	if (m_definition->m_animationGroups.size() > 0)
	{
		m_defaultAnimationGroup = m_definition->m_animationGroups[0];
		m_animationGroup = m_defaultAnimationGroup;
		m_animTimer->m_period = m_animationGroup.m_secondsPerFrame;
		m_animTimer->Start();
	}

	if (m_definition->m_dieOnSpawn)
	{
		Die();
	}
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

	// Reset to default if play once animations have finished
	if (m_animationGroup.m_playbackMode == SpriteAnimPlaybackType::ONCE && m_animTimer->HasPeriodElapsed())
	{
		SetAnimGroup(m_defaultAnimationGroup);
	}

	if (m_animationGroup.m_scaleBySpeed)
	{
		float velocityFraction = Vec2(m_velocity.x, m_velocity.y).GetLength() / m_definition->m_runSpeed;
		m_animClock->SetTimeScale(velocityFraction);
	}
	else
	{
		m_animClock->SetTimeScale(1.f);
	}

	m_isGrounded = false;
}

void Actor::Render()
{
	Player* currentlyRenderedPlayer = m_map->GetCurrentRenderedPlayer();

	if (
	!m_map->m_isRenderingPortal &&
	(m_controller != nullptr && m_controller->IsPlayer() && currentlyRenderedPlayer->m_desiredPlayerState == PlayerState::FIRSTPERSON && currentlyRenderedPlayer->m_playerIndex == ((Player*)m_controller)->m_playerIndex) ||
	!m_definition->m_visible
		)
	{
		return;
	}

	// Set proper UVs
	if (m_definition->m_spriteSheet != nullptr)
	{
		float largestDotProduct = -1.f;
		ActorDefinition::AnimationGroup::Animation animationInUse = m_animationGroup.m_animations[0];
		for (int animationIndex = 0; animationIndex < m_animationGroup.m_animations.size(); ++animationIndex)
		{
			ActorDefinition::AnimationGroup::Animation currentAnimation = m_animationGroup.m_animations[animationIndex];
			Vec3 actorToCamera = g_engine->m_render->GetCamera()->GetPosition() - m_position; // TODO: Put this in a non const render function so that it can properly use current rendered player.
			float curDotProduct = DotProduct3D(-actorToCamera, currentAnimation.m_direction.GetRotatedAboutZDegrees(m_orientation.m_yawDegrees));
			if (curDotProduct > largestDotProduct)
			{
				largestDotProduct = curDotProduct;
				animationInUse = currentAnimation;
			}
		}
		int currentSpriteIndex = animationInUse.m_startFrame;
		SpriteDef spriteDef = animationInUse.m_animDef->GetSpriteDefAtTime(m_animTimer->GetElapsedTime());
		if (m_definition->m_renderRounded)
		{
			AABB2 spriteUVs = spriteDef.m_UVs;
			float halfWidth = spriteUVs.GetWidth() * 0.5f;
			//float halfHeight = spriteUVs.GetHeight() * 0.5f;
			m_verts[0].m_uvTexCoords = spriteUVs.m_mins;
			m_verts[1].m_uvTexCoords = Vec2(spriteUVs.m_mins.x + halfWidth, spriteUVs.m_mins.y);
			m_verts[2].m_uvTexCoords = Vec2(spriteUVs.m_mins.x + halfWidth, spriteUVs.m_maxs.y);
			m_verts[3].m_uvTexCoords = Vec2(spriteUVs.m_mins.x, spriteUVs.m_maxs.y);
			m_verts[4].m_uvTexCoords = Vec2(spriteUVs.m_maxs.x, spriteUVs.m_mins.y);
			m_verts[5].m_uvTexCoords = spriteUVs.m_maxs;
		}
		else
		{
			AABB2 spriteUVs = spriteDef.m_UVs;
			m_verts[0].m_uvTexCoords = spriteUVs.m_mins;
			m_verts[1].m_uvTexCoords = Vec2(spriteUVs.m_maxs.x, spriteUVs.m_mins.y);
			m_verts[2].m_uvTexCoords = spriteUVs.m_maxs;
			m_verts[3].m_uvTexCoords = Vec2(spriteUVs.m_mins.x, spriteUVs.m_maxs.y);
		}
	}

	g_engine->m_render->BindShader(m_definition->m_shader);
	g_engine->m_render->SetModelConstants(GetModelMatrixBillboarded());
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_engine->m_render->BindTexture(m_definition->m_spriteSheet->GetTexture());
	g_engine->m_render->DrawIndexedVertexList(&m_verts, &m_vertexIndexes, m_map->GetVertexBuffer(), m_map->GetIndexBuffer());

	//Render_Debug();
}

void Actor::Render_Debug() const
{
	g_engine->m_render->SetModelConstants(GetModelMatrixOnlyYaw());
	std::vector<Vertex> debugVerts;

	Rgba8 colorToUse = m_color;
	if (m_isDead)
	{
		colorToUse.ScaleColor(0.5f);
	}

	AddVertsForCylinder3D(debugVerts, Vec3(), Vec3(0.f, 0.f, m_definition->m_height), m_definition->m_radius, colorToUse, AABB2::ZERO_TO_ONE, 16);
	Vec3 displacementFromCenter = Vec3(m_definition->m_radius, 0.f, 0.f);
	displacementFromCenter.GetRotatedAboutZDegrees(m_orientation.m_yawDegrees);

	if (!m_definition->m_isFlying)
	{
		Vec3 noseStart = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter;
		Vec3 noseEnd = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter + displacementFromCenter * 0.3f;
		AddVertsForCone3D(debugVerts, noseStart, noseEnd, 0.1f, colorToUse, AABB2::ZERO_TO_ONE, 16);
	}

	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->DrawVertexList(&debugVerts);

	debugVerts.clear();
	AddVertsForCylinder3D(debugVerts, Vec3(), Vec3(0.f, 0.f, m_definition->m_height + 0.001f), m_definition->m_radius + 0.001f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16);

	if (!m_definition->m_isFlying)
	{
		Vec3 noseStart = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter;
		Vec3 noseEnd = Vec3(0.f, 0.f, m_definition->m_eyeHeight - 0.1f) + displacementFromCenter + displacementFromCenter * 0.3f;
		AddVertsForCone3D(debugVerts, noseStart, noseEnd, 0.1f, Rgba8::WHITE, AABB2::ZERO_TO_ONE, 16);
	}

	g_engine->m_render->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	g_engine->m_render->DrawVertexList(&debugVerts);
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

Mat44 Actor::GetModelMatrixBillboarded() const
{
	Mat44 modelToWorld = Mat44();

	//modelToWorld.AppendTranslation3D(m_position);

	/*Mat44 billboardMatrix = GetBillboardTransform(m_definition->m_billboardType, m_map->m_currentlyRenderedPlayer->GetModelToWorldTransform(), m_position);*/
	Mat44 cameraTransform = g_engine->m_render->GetCamera()->GetCameraToWorldTransform();
	Mat44 billboardMatrix = GetBillboardTransform(m_definition->m_billboardType, cameraTransform, m_position);
	modelToWorld.Append(billboardMatrix);

	return modelToWorld;
}

Vec3 Actor::GetEyePos()
{
	return m_position + Vec3(0.f, 0.f, m_definition->m_eyeHeight);
}

int Actor::GetEquippedWeaponIndex()
{
	for (int weaponIndex = 0; weaponIndex < m_weapons.size(); ++weaponIndex)
	{
		if (m_weapons[weaponIndex] == m_equippedWeapon)
		{
			return weaponIndex;
		}
	}
	return -1;
}

void Actor::Update_Physics()
{
	if (m_definition->m_physicsIsSimulated && !m_isDead)
	{
		float deltaSeconds = (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
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

		AddForce(m_definition->m_drag * m_velocity * -1);

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
		m_coyoteTime += (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
	}

	if (m_deathTimer->HasPeriodElapsed())
	{
		m_isGarbage = true;
	}
}

void Actor::Update_Position()
{
	if (m_definition->m_physicsIsSimulated)
	{
		m_position = m_desiredPosition;
	}
}

void Actor::SetActorHandle(ActorHandle* handle)
{
	m_handle = handle;
}

void Actor::SetAnimGroup(std::string animGroupName)
{
	if (m_animationGroup.m_name == animGroupName)
	{
		return;
	}
	for (ActorDefinition::AnimationGroup curAnimGroup : m_definition->m_animationGroups)
	{
		if (curAnimGroup.m_name == animGroupName)
		{
			m_animationGroup = curAnimGroup;
			m_animTimer->m_period = curAnimGroup.m_secondsPerFrame * (m_animationGroup.m_animations[0].m_endFrame - m_animationGroup.m_animations[0].m_startFrame); // Just take the first animations frame count, all the animations in a group happen to have the same frame count so it should be ok.
			m_animTimer->Start();
			return;
		}
	}
}

void Actor::SetAnimGroup(ActorDefinition::AnimationGroup animGroup)
{
	m_animationGroup = animGroup;
	m_animTimer->m_period = animGroup.m_secondsPerFrame * (m_animationGroup.m_animations[0].m_endFrame - m_animationGroup.m_animations[0].m_startFrame);
	m_animTimer->Start();
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
	if (!m_isDead && m_coyoteTime < m_coyoteTimeMax)
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
	SetAnimGroup("Hurt");
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
		SetAnimGroup("Death");
	}
}

void Actor::OnCollide(Actor* otherActor)
{
	if (otherActor != nullptr && m_definition->m_damageOnCollide != FloatRange(-1, -1))
	{
		float damage = m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_damageOnCollide.m_min, m_definition->m_damageOnCollide.m_max);
		otherActor->Damage(RoundDownToInt(damage), m_owner->m_handle);
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
		newActorDef->m_visible = ParseXmlAttribute(*actorDefElement, "visible", false);
		newActorDef->m_dieOnSpawn = ParseXmlAttribute(*actorDefElement, "dieOnSpawn", false);

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

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Visuals
		XmlElement* VisualsElement = actorDefElement->FirstChildElement("Visuals");

		if (VisualsElement != nullptr)
		{
			newActorDef->m_size = ParseXmlAttribute(*VisualsElement, "size", Vec2());
			newActorDef->m_pivot = ParseXmlAttribute(*VisualsElement, "pivot", Vec2());
			std::string billboardName = ParseXmlAttribute(*VisualsElement, "billboardType", "");
			if (billboardName == "WorldUpFacing")
			{
				newActorDef->m_billboardType = BillboardType::WORLD_UP_FACING;
			}
			else if (billboardName == "WorldUpOpposing")
			{
				newActorDef->m_billboardType = BillboardType::WORLD_UP_OPPOSING;
			}
			else if (billboardName == "FullFacing")
			{
				newActorDef->m_billboardType = BillboardType::FULL_FACING;
			}
			else if (billboardName == "FullOpposing")
			{
				newActorDef->m_billboardType = BillboardType::FULL_OPPOSING;
			}
			newActorDef->m_renderLit = ParseXmlAttribute(*VisualsElement, "renderLit", false);
			newActorDef->m_renderRounded = ParseXmlAttribute(*VisualsElement, "renderRounded", false);
			std::string shaderPath = ParseXmlAttribute(*VisualsElement, "shader", "");
			newActorDef->m_shader = g_engine->m_render->CreateOrGetShader(shaderPath.c_str(), VertexType::VERTEX_PCUTBN);
			std::string spriteSheetPath = ParseXmlAttribute(*VisualsElement, "spriteSheet", "");
			newActorDef->m_cellCount = ParseXmlAttribute(*VisualsElement, "cellCount", IntVec2());
			newActorDef->m_spriteSheet = new SpriteSheet(g_engine->m_render->CreateOrGetTextureFromFile(spriteSheetPath.c_str()), newActorDef->m_cellCount);
		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// AnimationGroup
		if (VisualsElement != nullptr)
		{
			XmlElement* AnimGroupElement = VisualsElement->FirstChildElement();

			while (AnimGroupElement)
			{
				ActorDefinition::AnimationGroup animGroup = ActorDefinition::AnimationGroup();
				animGroup.m_name = ParseXmlAttribute(*AnimGroupElement, "name", "");
				animGroup.m_secondsPerFrame = ParseXmlAttribute(*AnimGroupElement, "secondsPerFrame", -1.f);
				std::string playbackMode = ParseXmlAttribute(*AnimGroupElement, "playbackMode", "");
				animGroup.m_scaleBySpeed = ParseXmlAttribute(*AnimGroupElement, "scaleBySpeed", false);
				if (playbackMode == "Once")
				{
					animGroup.m_playbackMode = SpriteAnimPlaybackType::ONCE;
				}
				else if (playbackMode == "Loop")
				{
					animGroup.m_playbackMode = SpriteAnimPlaybackType::LOOP;
				}
				else if (playbackMode == "PingPong")
				{
					animGroup.m_playbackMode = SpriteAnimPlaybackType::PINGPONG;
				}

				XmlElement* AnimationElement = AnimGroupElement->FirstChildElement();

				while (AnimationElement)
				{
					ActorDefinition::AnimationGroup::Animation animation = ActorDefinition::AnimationGroup::Animation();

					animation.m_direction = ParseXmlAttribute(*AnimationElement, "vector", Vec3()).GetNormalized(); // Direction needs to be normalized.
					XmlElement* AnimationDataElement = AnimationElement->FirstChildElement();
					animation.m_startFrame = ParseXmlAttribute(*AnimationDataElement, "startFrame", -1);
					animation.m_endFrame = ParseXmlAttribute(*AnimationDataElement, "endFrame", -1);

					SpriteAnimDefinition* newAnimDef = new SpriteAnimDefinition(*newActorDef->m_spriteSheet, animation.m_startFrame, animation.m_endFrame, 1.f / animGroup.m_secondsPerFrame, animGroup.m_playbackMode);
					animation.m_animDef = newAnimDef;

					animGroup.m_animations.push_back(animation);
					AnimationElement = AnimationElement->NextSiblingElement();
				}

				newActorDef->m_animationGroups.push_back(animGroup);
				AnimGroupElement = AnimGroupElement->NextSiblingElement();
			}
		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Sound
		XmlElement* SoundsElement = actorDefElement->FirstChildElement("Sounds");
		if (SoundsElement != nullptr)
		{
			XmlElement* SoundElement = SoundsElement->FirstChildElement();

			while (SoundElement)
			{
				ActorDefinition::Sound newSound = ActorDefinition::Sound();

				newSound.m_name = ParseXmlAttribute(*SoundElement, "sound", "");
				std::string soundPath = ParseXmlAttribute(*SoundElement, "name", "");
				newSound.m_sound = g_engine->m_audio->CreateOrGetSound(soundPath);

				newActorDef->m_sounds.push_back(newSound);
				SoundElement = SoundElement->NextSiblingElement();
			}
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

ActorDefinition::AnimationGroup::Animation::~Animation()
{
}
