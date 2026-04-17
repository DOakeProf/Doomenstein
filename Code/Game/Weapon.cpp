#include "Game/Weapon.hpp"
#include "Engine/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/DebugRender.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Portal.hpp"

std::vector<WeaponDefinition*> WeaponDefinition::s_definitions;

void WeaponDefinition::InitializeDefinitions(const char* path)
{
	XmlDocument tileDefsXml;
	[[maybe_unused]] XmlResult result = tileDefsXml.LoadFile(path);
	XmlElement* rootElement = tileDefsXml.RootElement();
	XmlElement* weaponDefElement = rootElement->FirstChildElement();

	while (weaponDefElement)
	{
		WeaponDefinition* newWeaponDef = new WeaponDefinition();
		
		newWeaponDef->m_name =						ParseXmlAttribute(*weaponDefElement, "name", "");
		if (newWeaponDef->m_name == "PortalGun")
		{
			newWeaponDef->m_type = WeaponType::PORTALGUN;
		}
		else
		{
			newWeaponDef->m_type = WeaponType::WEAPON;
		}

		newWeaponDef->m_refireTime =				ParseXmlAttribute(*weaponDefElement, "refireTime", -1.f);

		newWeaponDef->m_rayCount =					ParseXmlAttribute(*weaponDefElement, "rayCount", -1);
		newWeaponDef->m_rayCone =					ParseXmlAttribute(*weaponDefElement, "rayCone", -1.f);
		newWeaponDef->m_rayRange =					ParseXmlAttribute(*weaponDefElement, "rayRange", -1.f);
		newWeaponDef->m_rayDamage =					ParseXmlAttribute(*weaponDefElement, "rayDamage", FloatRange());
		newWeaponDef->m_rayImpulse =				ParseXmlAttribute(*weaponDefElement, "rayImpulse", -1.f);

		newWeaponDef->m_projectileCount =			ParseXmlAttribute(*weaponDefElement, "projectileCount", -1);
		newWeaponDef->m_projectileActor =			ParseXmlAttribute(*weaponDefElement, "projectileActor", "");
		newWeaponDef->m_secondaryProjectileActor =	ParseXmlAttribute(*weaponDefElement, "secondaryProjectileActor", "");
		newWeaponDef->m_projectileCone =			ParseXmlAttribute(*weaponDefElement, "projectileCone", 0.f);
		newWeaponDef->m_projectileSpeed =			ParseXmlAttribute(*weaponDefElement, "projectileSpeed", -1.f);

		newWeaponDef->m_meleeCount =				ParseXmlAttribute(*weaponDefElement, "meleeCount", -1);
		newWeaponDef->m_meleeArc =					ParseXmlAttribute(*weaponDefElement, "meleeArc", -1.f);
		newWeaponDef->m_meleeRange =				ParseXmlAttribute(*weaponDefElement, "meleeRange", -1.f);
		newWeaponDef->m_meleeDamage =				ParseXmlAttribute(*weaponDefElement, "meleeDamage", FloatRange());
		newWeaponDef->m_meleeImpulse =				ParseXmlAttribute(*weaponDefElement, "meleeImpulse", -1.f);

		newWeaponDef->m_portalHeight = ParseXmlAttribute(*weaponDefElement, "portalHeight", -1.f);
		newWeaponDef->m_portalWidth = ParseXmlAttribute(*weaponDefElement, "portalWidth", -1.f);

		//XmlElement* collisionElement = weaponDefElement->FirstChildElement("Collision");
		//if (collisionElement != nullptr)
		//{
		//	newWeaponDef->m_radius = ParseXmlAttribute(*collisionElement, "radius", -1.f);
		//}

		s_definitions.push_back(newWeaponDef);
		weaponDefElement = weaponDefElement->NextSiblingElement();
	}
}

void WeaponDefinition::ClearDefinitions()
{
	s_definitions.clear();
}

const WeaponDefinition* WeaponDefinition::GetByName(const std::string& name)
{
	for (int mapIndex = 0; mapIndex < s_definitions.size(); ++mapIndex)
	{
		WeaponDefinition* currentDef = s_definitions[mapIndex];
		if (currentDef->m_name == name)
		{
			return currentDef;
		}
	}
	return nullptr;
}

Weapon::Weapon(Map* map, std::string definition)
	: m_map(map)
{
	m_definition = WeaponDefinition::GetByName(definition);

	m_fireTimer = new Timer(m_definition->m_refireTime, m_map->m_game->m_gameClock);
	m_fireTimer->Start();
	m_alternateFireTimer = new Timer(m_definition->m_refireTime, m_map->m_game->m_gameClock);
	m_alternateFireTimer->Start();
}

Weapon::~Weapon()
{
	delete m_fireTimer;
	m_fireTimer = nullptr;
}

void Weapon::Fire(Actor* actor)
{
	if (!m_fireTimer->DecrementPeriodIfElapsed())
	{
		return;
	}
	else
	{
		m_fireTimer->Start();
	}
	switch (m_definition->m_type)
	{
		case WeaponType::WEAPON:	Fire_Weapon(actor);		break;
		case WeaponType::PORTALGUN: Fire_PortalGun(actor);	break;
	}
}

void Weapon::AlternateFire(Actor* actor)
{
	if (!m_alternateFireTimer->DecrementPeriodIfElapsed())
	{
		return;
	}
	else
	{
		m_alternateFireTimer->Start();
	}
	switch (m_definition->m_type)
	{
		case WeaponType::WEAPON:	AlternateFire_Weapon(actor);		break;
		case WeaponType::PORTALGUN: AlternateFire_PortalGun(actor);		break;
	}
}

void Weapon::AlternateFire_Weapon(Actor* actor)
{

}

void Weapon::AlternateFire_PortalGun(Actor* actor)
{
	if (m_definition->m_rayCount != -1)
	{
		for (int rayIndex = 0; rayIndex < m_definition->m_rayCount; ++rayIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_rayCone);
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);

			RaycastResultDoomenstein result = actor->m_map->RaycastAll(initialFirePosition, randomDirection, m_definition->m_rayRange, actor);
			if (result.m_didImpact && result.m_actor != nullptr)
			{
				float RandomDamage = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
				result.m_actor->Damage(RandomDamage, actor->m_handle);
				result.m_actor->AddImpulse(randomDirection * m_definition->m_rayImpulse);
			}
			else if (result.m_didImpact)
			{
				if (m_rightPortal != nullptr)
				{
					m_map->RemovePortal(m_rightPortal);
					m_rightPortal = nullptr;
				}
				//Vec3 portalPosition = actor->m_position + actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f;

				PushImpactPointToFitSurface(result);

				Vec3 portalPosition = result.m_impactPos + result.m_impactNormal * 0.0001;
				m_rightPortal = new Portal(m_map, portalPosition, m_definition->m_portalHeight, m_definition->m_portalWidth);
				m_rightPortal->m_isFlipped = true;

				Vec3 inverseImpactNormal = result.m_impactNormal * -1.f; // Flip the right portal so that it is directing towards the surface its on. This will make the link between both portals shoot things that are entering away from the walls instead of towards them.
				if (inverseImpactNormal.z == 0.f) // If the impact normal is horizontal
				{
					Mat44 portalOrientation = Mat44(inverseImpactNormal, CrossProduct3D(Vec3(0.f, 0.f, 1.f), inverseImpactNormal), Vec3(0.f, 0.f, 1.f), Vec3());
					m_rightPortal->SetOrientation(EulerAngles(portalOrientation));
				}
				else // If the impact normal is vertical
				{
					//Vec3 iBasis = inverseImpactNormal;
					//Vec3 jBasis = CrossProduct3D(inverseImpactNormal, actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp());
					//Vec3 kBasis = CrossProduct3D(jBasis, inverseImpactNormal);

					//EulerAngles rightPortalOrientation = EulerAngles();
					//rightPortalOrientation.m_yawDegrees = Atan2Degrees(iBasis.y, iBasis.x);
					//rightPortalOrientation.m_pitchDegrees = AsinDegrees(-iBasis.z);

					//if (iBasis.z == -1.f)
					//{
					//	kBasis *= -1.f;
					//}

					//rightPortalOrientation.m_rollDegrees = Atan2Degrees(kBasis.y, kBasis.x);

					EulerAngles rightPortalOrientation = actor->m_orientation;

					if (result.m_impactNormal.z == 1.f)
					{
						rightPortalOrientation.m_pitchDegrees = 90.f;
					}
					else
					{
						rightPortalOrientation.m_pitchDegrees = -90.f;
					}

					m_rightPortal->SetOrientation(rightPortalOrientation);
				}

				m_rightPortal->AssignPortal(m_leftPortal);
				if (m_leftPortal != nullptr)
				{
					m_leftPortal->AssignPortal(m_rightPortal);
				}

				m_map->AddPortal(m_rightPortal);
			}
		}
	}
}

void Weapon::PushImpactPointToFitSurface(RaycastResultDoomenstein& result)
{
	// Push the portal to fit onto whatever surface its on.
	if (result.m_impactNormal.z == 0.f) // Is a vertical (wall) portal
	{
		int tileIndex = m_map->GetTileIndexFromWorldPosition(result.m_impactPos + result.m_impactNormal * 0.01f);
		const Tile* tileInFrontOfPortal = m_map->GetTile(tileIndex);
		float halfPortalHeight = m_definition->m_portalHeight * 0.5f;
		float halfPortalWidth = m_definition->m_portalWidth * 0.5f;

		// Has a floor and portal is over it
		if (tileInFrontOfPortal->m_tileDefinition->m_floorSpriteCoords != IntVec2(-1.f, -1.f) &&
			result.m_impactPos.z - halfPortalHeight < tileInFrontOfPortal->m_bounds.m_mins.z)
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_mins.z + halfPortalHeight;
		}
		// Has a Ceiling and portal is over it
		if (tileInFrontOfPortal->m_tileDefinition->m_ceilingSpriteCoords != IntVec2(-1.f, -1.f) &&
			result.m_impactPos.z + halfPortalHeight > tileInFrontOfPortal->m_bounds.m_maxs.z)
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_maxs.z - halfPortalHeight;
		}

		Vec3 rotatedImpactNormal = result.m_impactNormal.GetRotatedAboutZDegrees(90.f);
		Vec3 rotatedWidthVec = rotatedImpactNormal * halfPortalWidth;
		tileIndex = m_map->GetTileIndexFromWorldPosition(result.m_impactPos + result.m_impactNormal * -0.01f + Vec3(0.f, 0.f, 1.f));
		const Tile* tileAbovePortal = m_map->GetTile(tileIndex);
		tileIndex = m_map->GetTileIndexFromWorldPosition(result.m_impactPos + result.m_impactNormal * -0.01f - Vec3(0.f, 0.f, 1.f));
		const Tile* tileBelowPortal = m_map->GetTile(tileIndex);

		Vec3 rightTilePos = (result.m_impactPos + result.m_impactNormal * -0.01f + rotatedImpactNormal);
		Vec3 leftTilePos = (result.m_impactPos + result.m_impactNormal * -0.01f - rotatedImpactNormal);
		tileIndex = m_map->GetTileIndexFromWorldPosition(rightTilePos);
		const Tile* tileRightOfPortal = m_map->GetTile(tileIndex);
		tileIndex = m_map->GetTileIndexFromWorldPosition(leftTilePos);
		const Tile* tileLeftOfPortal = m_map->GetTile(tileIndex);

		// Portal hanging off top
		if (tileAbovePortal == nullptr ||
			(tileAbovePortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1.f, -1.f) &&
				result.m_impactPos.z + halfPortalHeight > tileAbovePortal->m_bounds.m_mins.z))
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_maxs.z - halfPortalHeight;
		}
		// Portal hanging off bottom
		if (tileBelowPortal == nullptr ||
			(tileBelowPortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1.f, -1.f) &&
				result.m_impactPos.z + halfPortalHeight < tileBelowPortal->m_bounds.m_maxs.z))
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_mins.z + halfPortalHeight;
		}
		// Portal hanging off right side
		if (tileRightOfPortal == nullptr ||
			(tileRightOfPortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1.f, -1.f) &&
				IsPointInsideAABB3D((result.m_impactPos + result.m_impactNormal * -0.01f + rotatedWidthVec), tileRightOfPortal->m_bounds)))
		{
			if (result.m_impactNormal.x == 0) // Portal is along x axis
			{
				if (fmod(result.m_impactPos.x, 1.f) > 0.5f)
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) + halfPortalWidth;
				}
			}
			else // Portal is along y axis
			{
				if (fmod(result.m_impactPos.y, 1.f) > 0.5f)
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) + halfPortalWidth;
				}
			}
		}
		// Portal hanging off left side
		if (tileLeftOfPortal == nullptr ||
			(tileLeftOfPortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1.f, -1.f) &&
				IsPointInsideAABB3D((result.m_impactPos + result.m_impactNormal * -0.01f - rotatedWidthVec), tileLeftOfPortal->m_bounds)))
		{
			if (result.m_impactNormal.x == 0) // Portal is along x axis
			{
				if (fmod(result.m_impactPos.x, 1.f) > 0.5f)
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) + halfPortalWidth;
				}
			}
			else // Portal is along y axis
			{
				if (fmod(result.m_impactPos.y, 1.f) > 0.5f)
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) + halfPortalWidth;
				}
			}
		}
	}
	else // Is a horizontal (floor/ceiling) portal
	{

	}
}

void Weapon::Fire_Weapon(Actor* actor)
{
	if (m_definition->m_rayCount != -1)
	{
		for (int rayIndex = 0; rayIndex < m_definition->m_rayCount; ++rayIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_rayCone);
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);

			RaycastResultDoomenstein result = actor->m_map->RaycastAll(initialFirePosition, randomDirection, m_definition->m_rayRange, actor);
			if (result.m_didImpact && result.m_actor != nullptr)
			{
				float RandomDamage = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
				result.m_actor->Damage(RandomDamage, actor->m_handle);
				result.m_actor->AddImpulse(randomDirection * m_definition->m_rayImpulse);
			}
		}
	}
	if (!m_definition->m_projectileActor.empty())
	{
		Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_projectileCone);
		Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight - 0.1f);

		SpawnInfo spawnInfo = SpawnInfo(m_definition->m_projectileActor, initialFirePosition, actor->m_orientation);
		Actor* projectile = actor->m_map->SpawnActor(spawnInfo);
		projectile->m_owner = actor;
		projectile->AddImpulse(randomDirection * m_definition->m_projectileSpeed);
	}
	if (m_definition->m_meleeCount != -1)
	{
		std::vector<Actor*> actors = m_map->GetActors();
		for (int actorIndex = 0; actorIndex < actors.size(); ++actorIndex)
		{
			Actor* currentActor = actors[actorIndex];
			bool isActorSameFaction = m_map->AreActorsSameFaction(actor, currentActor);

			if (currentActor != nullptr && currentActor != actor && !isActorSameFaction)
			{
				Vec3 distFromSelfToOther = currentActor->m_position - actor->m_position;
				float angleBetweenSelfToOtherAndForward = ConvertRadiansToDegrees(GetAngleDegreesBetweenVectors3D(distFromSelfToOther.GetNormalized(), currentActor->m_orientation.GetForwardDir_IFwd_JLeft_KUp()));
				if (distFromSelfToOther.GetLength() < m_definition->m_meleeRange &&
					angleBetweenSelfToOtherAndForward < m_definition->m_meleeArc)
				{
					float RandomDamage = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_meleeDamage.m_min, m_definition->m_meleeDamage.m_max);
					currentActor->Damage(RandomDamage, actor->m_handle);
					currentActor->AddImpulse(distFromSelfToOther.GetNormalized() * m_definition->m_meleeImpulse);
				}
			}
		}
	}
}

void Weapon::Fire_PortalGun(Actor* actor)
{
	if (m_definition->m_rayCount != -1)
	{
		for (int rayIndex = 0; rayIndex < m_definition->m_rayCount; ++rayIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_rayCone);
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);

			RaycastResultDoomenstein result = actor->m_map->RaycastAll(initialFirePosition, randomDirection, m_definition->m_rayRange, actor);
			if (result.m_didImpact && result.m_actor != nullptr)
			{
				float RandomDamage = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
				result.m_actor->Damage(RandomDamage, actor->m_handle);
				result.m_actor->AddImpulse(randomDirection * m_definition->m_rayImpulse);
			}
			else if (result.m_didImpact)
			{
				if (m_leftPortal != nullptr)
				{
					m_map->RemovePortal(m_leftPortal);
					m_leftPortal = nullptr;
				}
				//Vec3 portalPosition = actor->m_position + actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f;

				PushImpactPointToFitSurface(result);

				Vec3 portalPosition = result.m_impactPos + result.m_impactNormal * 0.0001;
				m_leftPortal = new Portal(m_map, portalPosition, m_definition->m_portalHeight, m_definition->m_portalWidth);
				
				if (result.m_impactNormal.z == 0.f) // If the impact normal is horizontal
				{
					Mat44 portalOrientation = Mat44(result.m_impactNormal, CrossProduct3D(Vec3(0.f,0.f,1.f), result.m_impactNormal), Vec3(0.f,0.f,1.f), Vec3());
					m_leftPortal->SetOrientation(EulerAngles(portalOrientation));
				}
				else // If the impact normal is vertical
				{
					//Vec3 iBasis = result.m_impactNormal;
					//Vec3 jBasis = CrossProduct3D(iBasis, actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp());
					//Vec3 kBasis = CrossProduct3D(jBasis, iBasis);

					//EulerAngles leftPortalOrientation = EulerAngles();
					//leftPortalOrientation.m_yawDegrees = Atan2Degrees(iBasis.y, iBasis.x);
					//leftPortalOrientation.m_pitchDegrees = AsinDegrees(-iBasis.z);

					//if (iBasis.z == 1.f)
					//{
					//	kBasis *= -1.f;
					//}

					//leftPortalOrientation.m_rollDegrees = Atan2Degrees(kBasis.y, kBasis.x);

					EulerAngles leftPortalOrientation = actor->m_orientation;

					if (result.m_impactNormal.z == 1.f)
					{
						leftPortalOrientation.m_pitchDegrees = -90.f;
					}
					else
					{
						leftPortalOrientation.m_pitchDegrees = 90.f;
					}

					m_leftPortal->SetOrientation(leftPortalOrientation);
				}

				m_leftPortal->AssignPortal(m_rightPortal);
				if (m_rightPortal != nullptr)
				{
					m_rightPortal->AssignPortal(m_leftPortal);
				}

				m_map->AddPortal(m_leftPortal);
			}
		}
	}
}
