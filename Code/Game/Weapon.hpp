#pragma once

#include <string>
#include <vector>

#include "Game/Map.hpp"

#include "Engine/Core/Timer.hpp"
#include "Engine/Math/FloatRange.hpp"

class Actor;

struct WeaponDefinition
{
	std::string m_name;
	float m_refireTime;

	int m_rayCount;
	float m_rayCone;
	float m_rayRange;
	FloatRange m_rayDamage;
	float m_rayImpulse;

	int m_projectileCount;
	std::string m_projectileActor;
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

	void Fire(Actor* actor);
};