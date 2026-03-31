#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"

struct Mat44;

class Actor
{
public:
	Actor(Vec3 const& position, float height, float radius, EulerAngles const& orientation = EulerAngles(), Rgba8 const& color = Rgba8::WHITE);
	~Actor();

	Vec3 m_position;
	EulerAngles m_orientation;
	Rgba8 m_color;
	float m_physicsHeight;
	float m_physicsRadius;
	bool m_isStatic = false;

	void Update();
	void Render() const;
	Mat44 GetModelMatrix() const;

	void setStatic(bool status);
};