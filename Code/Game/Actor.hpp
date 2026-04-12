#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Math/FloatRange.hpp"

#include "Game/Weapon.hpp"

#include <string>

struct ActorHandle;
struct Mat44;
struct Vertex;
class Map;
class Controller;
class AI;

struct ActorDefinition
{
	std::string m_name;
	std::string m_faction;
	int m_health;
	bool m_canBePossessed;
	float m_corpseLifetime;
	bool m_visible;

	float m_radius;
	float m_height;
	bool m_collidesWithWorld;
	bool m_collidesWithActors;
	FloatRange m_damageOnCollide;
	float m_impulseOnCollide;
	bool m_dieOnCollide;
	bool m_collidesWithSameActor;

	bool m_physicsIsSimulated;
	float m_walkSpeed;
	float m_runSpeed;
	float m_turnSpeed;
	float m_drag;
	bool m_isFlying;

	float m_eyeHeight;
	float m_cameraFOV;

	bool m_aiEnabled;
	float m_sightRadius;
	float m_sightAngle;

	std::vector<std::string> m_inventory;

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
	std::vector<Vertex*> m_verts;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Gameplay
	int m_health = 1;
	std::vector<Weapon*> m_weapons; 
	Weapon* m_equippedWeapon;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Timers
	Timer* m_deathTimer;

	void Update();
	void Render() const;
	Mat44 GetModelMatrix() const;
	Mat44 GetModelMatrixOnlyYaw() const;

	void Update_Physics();
	void AddForce(Vec3 const& force);
	void AddImpulse(Vec3 const& impulse);
	void Update_Gameplay();
	void Update_Position();

	void SetActorHandle(ActorHandle* handle);

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