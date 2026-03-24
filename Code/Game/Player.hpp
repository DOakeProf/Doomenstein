#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"

class Game;
struct Camera;
struct Vec3;
struct Mat44;
struct EulerAngles;
struct Rgba8;

class Player
{
public:
	Player(Game* owner, Vec3 const& startingPosition);
	virtual ~Player() = default;

	virtual void Update();
	virtual void Render() const;
	virtual void Die();

	virtual Mat44 GetModelToWorldTransform() const;
	virtual Mat44 GetWorldToModelTransform() const;

	bool IsAlive() const;

	Camera* m_worldCamera = nullptr;
	Camera* m_screenCamera = nullptr;

	Vec3 m_position; // Not initialized like the others because by simply just defining a new object of these classes, they are already initialized in their constructors.
	Vec3 m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity;
	Rgba8 m_color = Rgba8::WHITE;
	bool m_isDead = false;
	bool m_isGarbage = false;
	Game* m_game = nullptr;
};