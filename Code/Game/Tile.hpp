#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB3.hpp"

#include <string>

struct TileDefinition
{
	std::string m_name;
	bool m_isSolid = false;
	Rgba8 m_mapImagePixelColor;
	IntVec2 m_floorSpriteCoords;
	IntVec2 m_ceilingSpriteCoords; 
	IntVec2 m_wallSpriteCoords;
	IntVec2 m_rampSpriteCoords;
	IntVec2 m_rampWallSpriteCoords;
	IntVec2 m_rampDirection;

	static void InitializeDefinitions(const char* path);
	static void ClearDefinitions();
	static const TileDefinition* GetByName(const std::string& name);
	static const TileDefinition* GetByPixelColor(Rgba8& color);
	static std::vector<TileDefinition*> s_definitions;
};

class Tile
{
public:
	Tile(const TileDefinition* tileDefinition, AABB3 m_bounds);
	~Tile() = default;

	const TileDefinition* m_tileDefinition;
	AABB3 m_bounds;
};