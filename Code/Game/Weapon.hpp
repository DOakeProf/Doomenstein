#pragma once

#include <string>
#include <vector>

#include "Game/Map.hpp"

#include "Engine/Core/Timer.hpp"
#include "Engine/Math/FloatRange.hpp"

class Actor;
class Portal;

enum class WeaponType
{
	NONE = -1,
	WEAPON,
	PORTALGUN,
	GRAPPLE, // TODO
	NUM_WEAPONTYPES
};

struct WeaponDefinition
{
	std::string m_name;
	WeaponType m_type;
	float m_refireTime;

	int m_rayCount;
	float m_rayCone;
	float m_rayRange;
	FloatRange m_rayDamage;
	float m_rayImpulse;

	int m_projectileCount;
	std::string m_projectileActor;
	std::string m_secondaryProjectileActor;
	float m_projectileCone;
	float m_projectileSpeed;

	int m_meleeCount;
	float m_meleeArc;
	float m_meleeRange;
	FloatRange m_meleeDamage;
	float m_meleeImpulse;

	static void InitializeDefinitions(const char* path);
	static void ClearDefinitions();
	static const WeaponDefinition* GetByName(const std::string& name);
	static std::vector<WeaponDefinition*> s_definitions;
};

class Weapon
{
public:
	Weapon(Map* map, std::string definition);
	~Weapon();

	Map* m_map;
	WeaponDefinition const* m_definition;
	Timer* m_fireTimer;
	Timer* m_alternateFireTimer;

	// Only for portal gun
	Portal* m_leftPortal;
	Portal* m_rightPortal;

	void Fire(Actor* actor);
	void Fire_Weapon(Actor* actor);
	void Fire_PortalGun(Actor* actor);

	void AlternateFire(Actor* actor);
	void AlternateFire_Weapon(Actor* actor);
	void AlternateFire_PortalGun(Actor* actor);
};