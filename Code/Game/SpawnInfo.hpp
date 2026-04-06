#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <String>

struct SpawnInfo
{
	std::string m_name;
	Vec3 m_position;
	EulerAngles m_orientation;
};