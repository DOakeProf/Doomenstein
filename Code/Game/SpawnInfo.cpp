#include "Game/SpawnInfo.hpp"

SpawnInfo::SpawnInfo(std::string name, Vec3 position, EulerAngles orientation, float size)
	: m_name(name)
	, m_position(position)
	, m_orientation(orientation)
	, m_size(size)
{

}
