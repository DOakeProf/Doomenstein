#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Audio/AudioSystem.hpp"

#include "Game/Weapon.hpp"
#include "Game/AI.hpp"

#include <string>

struct ActorHandle;
struct Mat44;
struct Vertex;
class Map;
class Controller;
class AI;

struct ActorDefinition
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Base
	std::string					m_name;
	std::string					m_faction;
	int							m_health;
	bool						m_canBePossessed;
	float						m_corpseLifetime;
	bool						m_visible;
	bool						m_dieOnSpawn;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Logic
	float						m_radius;
	float						m_height;
	bool						m_collidesWithWorld;
	bool						m_collidesWithActors;
	FloatRange					m_damageOnCollide;
	float						m_impulseOnCollide;
	bool						m_dieOnCollide;
	bool						m_collidesWithSameActor;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Physics
	bool						m_physicsIsSimulated;
	float						m_walkSpeed;
	float						m_runSpeed;
	float						m_turnSpeed;
	float						m_drag;
	bool						m_isFlying;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Player
	float						m_eyeHeight;
	float						m_cameraFOV;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// AI
	bool						m_aiEnabled;
	AIType						m_aiType;
	float						m_sightRadius;
	float						m_sightAngle;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Visuals
	Vec2						m_size;
	Vec2						m_pivot;
	BillboardType				m_billboardType;
	bool						m_renderLit;
	bool						m_renderRounded;
	Shader*						m_shader;
	SpriteSheet*				m_spriteSheet;
	IntVec2						m_cellCount;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// AnimationGroup
	struct AnimationGroup
	{
		std::string				m_name;
		float					m_secondsPerFrame;
		bool					m_scaleBySpeed;
		SpriteAnimPlaybackType	m_playbackMode;
		struct Animation
		{
			Animation() = default;
			~Animation();

			Vec3				m_direction;
			int					m_startFrame;
			int					m_endFrame;
			SpriteAnimDefinition* m_animDef;
		};
		std::vector<Animation> m_animations;
	};
	std::vector<ActorDefinition::AnimationGroup> m_animationGroups;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Sound
	struct Sound
	{
		std::string				m_name;
		SoundID					m_sound;
	};
	std::vector<Sound>			m_sounds;

	std::vector<std::string>	m_inventory;

	static void InitializeDefinitions(const char* path);
	static void ClearDefinitions();
	static const ActorDefinition* GetByName(const std::string& name);
	static std::vector<ActorDefinition*> s_definitions;
};

class Actor
{
public:
	Actor(Map* map, std::string name, Vec3 const& position, EulerAngles const& orientation = EulerAngles());
	~Actor();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Identity
	Map* m_map = nullptr;
	Actor* m_owner = nullptr; // Only applies to projectiles
	const ActorDefinition* m_definition;
	ActorHandle* m_handle = nullptr;
	Controller* m_controller = nullptr;
	AI* m_AIController = nullptr;
	bool m_isDead = false;
	bool m_isGarbage = false;
	bool m_hasEnteredRift = false;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement/Abilities
	bool m_isGrounded = false;
	bool m_isJumping = false;
	float m_coyoteTime = 0.f;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Physics
	Vec3 m_position;
	Vec3 m_desiredPosition; // For preventative physics
	EulerAngles m_orientation;
	Vec3 m_velocity;
	Vec3 m_acceleration;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Rendering
	Rgba8 m_color;
	std::vector<Vertex_PCUTBN> m_verts;
	std::vector<unsigned int> m_vertexIndexes;
	ActorDefinition::AnimationGroup m_animationGroup;
	ActorDefinition::AnimationGroup m_defaultAnimationGroup;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Gameplay
	int m_health = 1;
	std::vector<Weapon*> m_weapons; 
	Weapon* m_equippedWeapon;
	float m_coyoteTimeMax = 0.09f;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Timers
	Clock* m_animClock;
	Timer* m_deathTimer;
	Timer* m_animTimer;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Sound
	std::vector<SoundPlaybackID> m_soundPlaybackIDs;

	void Update();
	void Render();
	void Render_Debug() const;
	Mat44 GetModelMatrix() const;
	Mat44 GetModelMatrixOnlyYaw() const;
	Mat44 GetModelMatrixBillboarded() const;

	Vec3 GetEyePos();
	int GetEquippedWeaponIndex();

	void Update_Physics();
	void AddForce(Vec3 const& force);
	void AddImpulse(Vec3 const& impulse);
	void Update_Gameplay();
	void Update_Position();

	void SetActorHandle(ActorHandle* handle);
	void SetAnimGroup(std::string animGroupName);
	void SetAnimGroup(ActorDefinition::AnimationGroup animGroup);
	void PlaySoundOnActor(std::string soundName);
	void AddSoundPlaybackID(SoundPlaybackID playbackID);
	void ClearStoppedPlaybackID();

	void MoveInDirection(Vec3 const& direction, float speed);
	void TurnInDirection(float angleToTurnTowards, float maximumTurn);
	void Jump(float jumpStrength);
	void CancelJump();
	void Attack();
	void SecondaryAttack();
	void EquipWeapon(Weapon* weapon);
	void Damage(int damage, ActorHandle* otherActor);
	void Die();

	void OnCollide(Actor* otherActor);
	void OnPossessed();
	void OnUnpossessed();
};