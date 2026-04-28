#include "Game/Rift.hpp"

Rift::Rift(Map* map, Map* riftMap, Vec3 const& startingPosition, EulerAngles orientation, float height, float width)
	: Portal(map, startingPosition, orientation, height, width)
	, m_riftMap(riftMap)
{

}

Rift::~Rift()
{

}

