#pragma once

#include "Game/Portal.hpp"
#include "Engine/Math/Vec3.hpp"

class Map;

class Rift : public Portal 
{
public:
	Rift(Map* map, Map* riftMap, Vec3 const& startingPosition, EulerAngles orientation, float height, float width); // Spawn a SINGLE rift which exists in both maps?
	~Rift();
	
	Map* m_riftMap;
};