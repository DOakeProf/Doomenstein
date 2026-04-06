#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Timer.hpp"

struct ActorHandle;
struct Mat44;
class Map;

class Actor
{
public:
	Actor(Map* map, Vec3 const& position, float height, float radius, EulerAngles const& orientation = EulerAngles(), Rgba8 const& color = Rgba8::WHITE);
	~Actor();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Identity
	Map* m_map;
	ActorHandle* m_handle;
	bool m_isDead;
	bool m_isGarbage;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Rendering/Physics
	Vec3 m_position;
	EulerAngles m_orientation;
	Rgba8 m_color;
	float m_physicsHeight;
	float m_physicsRadius;
	bool m_isStatic = false;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Gameplay
	int m_health = 1;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Timers
	Timer* m_deathTimer;

	void Update();
	void Render() const;
	Mat44 GetModelMatrix() const;

	void SetActorHandle(ActorHandle* handle);

	void Die();

	void setStatic(bool status);
};