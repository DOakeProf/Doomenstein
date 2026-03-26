#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"

#include <string>

struct TileDefinition
{
	std::string m_name;
	bool m_isSolid = false;
	Rgba8 m_mapImagePixelColor;
	IntVec2 m_floorSpriteCoords;
	IntVec2 m_ceilingSpriteCoords; 
	IntVec2 m_wallSpriteCoords;

	static void InitializeDefinitions(const char* path);
	static void ClearDefinitions();
	static const TileDefinition* GetByName(const std::string& name);
	static std::vector<TileDefinition*> s_definitions;
};

class Tile
{

};