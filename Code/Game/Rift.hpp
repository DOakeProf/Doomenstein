#pragma once

#include "Game/Portal.hpp"
#include "Engine/Math/Vec3.hpp"

class Map;
struct ActorHandle;

class Rift : public Portal 
{
public:
	Rift(Vec3 const& startingPosition, EulerAngles orientation, float height, float width, float sizeScale = 1); // Spawn a SINGLE rift which exists in both maps?
	~Rift() override;
	
	void RenderRift(const Map* map);
	void RenderOutline(const Map* map);

	void Render_ActorsNearRift(const Map* map);

	std::vector<ActorHandle*> m_actorsNearRift;
	Map* m_riftMap;
};