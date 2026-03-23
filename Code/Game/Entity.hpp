#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Game/GameCommon.hpp"

class Game;

//----------------------------------------------------------------------------------------
class Entity
{
public:
	Vec3 m_position; // Not initialized like the others because by simply just defining a new object of these classes, they are already initialized in their constructors.
	Vec3 m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity;
	Rgba8 m_color = Rgba8::WHITE;
	bool m_isDead = false;
	bool m_isGarbage = false;
	Game* m_game = nullptr;

public:
	Entity(Game* owner, Vec3 const& startingPosition);
	virtual ~Entity() = default;

	virtual void Render() const = 0;
	virtual void Die();
	virtual void Update() = 0;

	virtual Mat44 GetModelToWorldTransform() const;
	virtual Mat44 GetWorldToModelTransform() const;

	bool IsAlive() const;
};