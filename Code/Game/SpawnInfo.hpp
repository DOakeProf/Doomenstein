#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <String>

struct SpawnInfo
{
	SpawnInfo(std::string name, Vec3 position, EulerAngles orientation);
	~SpawnInfo() = default;

	std::string m_name;
	Vec3 m_position;
	EulerAngles m_orientation;
};