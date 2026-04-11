#include "Game/Weapon.hpp"
#include "Engine/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"

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
	if (!m_definition->m_projectileActor.empty())
	{
		Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_projectileCone);
		Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight - 0.12f);

		SpawnInfo spawnInfo = SpawnInfo(m_definition->m_projectileActor, initialFirePosition, actor->m_orientation);
		Actor* projectile = actor->m_map->SpawnActor(spawnInfo);
		projectile->m_owner = actor;
		projectile->AddImpulse(randomDirection * m_definition->m_projectileSpeed);
	}
}

void Weapon::Fire_Weapon(Actor* actor)
{
	if (m_definition->m_rayCount != -1)
	{
		for (int rayIndex = 0; rayIndex < m_definition->m_rayCount; ++rayIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_rayCone);
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight - 0.001f);

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
		Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight - 0.12f);

		SpawnInfo spawnInfo = SpawnInfo(m_definition->m_projectileActor, initialFirePosition, actor->m_orientation);
		Actor* projectile = actor->m_map->SpawnActor(spawnInfo);
		projectile->m_owner = actor;
		projectile->AddImpulse(randomDirection * m_definition->m_projectileSpeed);
	}
	if (m_definition->m_meleeCount != -1)
	{

	}
}

void Weapon::Fire_PortalGun(Actor* actor)
{
	if (!m_definition->m_projectileActor.empty())
	{
		Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_projectileCone);
		Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight - 0.12f);

		SpawnInfo spawnInfo = SpawnInfo(m_definition->m_projectileActor, initialFirePosition, actor->m_orientation);
		Actor* projectile = actor->m_map->SpawnActor(spawnInfo);
		projectile->m_owner = actor;
		projectile->AddImpulse(randomDirection * m_definition->m_projectileSpeed);
	}
}
