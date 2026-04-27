#pragma once

#include <string>
#include <vector>

#include "Game/Map.hpp"

#include "Engine/Core/Timer.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

class Actor;
class Portal;
class glTF_Asset;

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
	std::string			m_name;
	WeaponType			m_type;
	float				m_refireTime;

	int					m_rayCount;
	float				m_rayCone;
	float				m_rayRange;
	FloatRange			m_rayDamage;
	float				m_rayImpulse;

	int					m_projectileCount;
	std::string			m_projectileActor;
	std::string			m_secondaryProjectileActor;
	float				m_projectileCone;
	float				m_projectileSpeed;

	int					m_meleeCount;
	float				m_meleeArc;
	float				m_meleeRange;
	FloatRange			m_meleeDamage;
	float				m_meleeImpulse;

	float				m_portalHeight;
	float				m_portalWidth;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// HUD
	Shader*				m_shader;
	Texture*			m_baseTexture;
	Texture*			m_reticleTexture;
	Vec2				m_reticleSize;
	IntVec2				m_spriteSize;
	Vec2				m_spritePivot;

	std::vector<glTF_Asset*> m_gltfAssets;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Animation
	struct Animation
	{
		std::string		m_name;
		Shader*			m_shader;
		SpriteSheet*	m_spriteSheet;
		IntVec2			m_cellCount;
		float			m_secondsPerFrame;
		int				m_startFrame;
		int				m_endFrame;

		SpriteAnimDefinition* m_animDef;
	};
	std::vector<Animation> m_animations;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Sound
	std::string			m_soundName;
	SoundID				m_sound;

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

	Timer* m_animTimer;
	WeaponDefinition::Animation m_animation;
	WeaponDefinition::Animation m_defaultAnimation;


	// Only for portal gun
	Portal* m_leftPortal;
	Portal* m_rightPortal;

	void Update();
	void Render();

	void Render_GLTF();

	void Fire(Actor* actor);
	void Fire_Weapon(Actor* actor);
	void Fire_PortalGun(Actor* actor);

	void AlternateFire(Actor* actor);
	void AlternateFire_Weapon(Actor* actor);
	void AlternateFire_PortalGun(Actor* actor);

	void SetAnimation(std::string animationName);
	void SetAnimation(WeaponDefinition::Animation animation);

	void PushImpactPointToFitSurface(RaycastResultDoomenstein& result);
};