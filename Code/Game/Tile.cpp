#include "Game/Tile.hpp"

#include "Engine/XmlUtils.hpp"
#include "Engine/Math/IntVec2.hpp"

std::vector<TileDefinition*> TileDefinition::s_definitions;

void TileDefinition::InitializeDefinitions(const char* path)
{
	XmlDocument tileDefsXml;
	[[maybe_unused]] XmlResult result = tileDefsXml.LoadFile(path);
	XmlElement* rootElement = tileDefsXml.RootElement();
	XmlElement* tileDefElement = rootElement->FirstChildElement();
	while (tileDefElement)
	{
		TileDefinition* newTileDef = new TileDefinition();
		newTileDef->m_name = ParseXmlAttribute(*tileDefElement, "name", "");
		newTileDef->m_isSolid = ParseXmlAttribute(*tileDefElement, "isSolid", false);
		newTileDef->m_mapImagePixelColor = ParseXmlAttribute(*tileDefElement, "mapImagePixelColor", Rgba8());
		newTileDef->m_wallSpriteCoords = ParseXmlAttribute(*tileDefElement, "wallSpriteCoords", IntVec2(-1, -1));
		newTileDef->m_floorSpriteCoords = ParseXmlAttribute(*tileDefElement, "floorSpriteCoords", IntVec2(-1, -1));
		newTileDef->m_ceilingSpriteCoords = ParseXmlAttribute(*tileDefElement, "ceilingSpriteCoords", IntVec2(-1, -1));
		newTileDef->m_rampSpriteCoords = ParseXmlAttribute(*tileDefElement, "rampSpriteCoords", IntVec2(-1, -1));
		newTileDef->m_rampDirection = ParseXmlAttribute(*tileDefElement, "rampDirection", IntVec2(-1, -1));
		s_definitions.push_back(newTileDef);
		tileDefElement = tileDefElement->NextSiblingElement();
	}
}

void TileDefinition::ClearDefinitions()
{
	s_definitions.clear();
}

const TileDefinition* TileDefinition::GetByName(const std::string& name)
{
	for (int tileIndex = 0; tileIndex < s_definitions.size(); ++tileIndex)
	{
		TileDefinition* currentDef = s_definitions[tileIndex];
		if (currentDef->m_name == name)
		{
			return currentDef;
		}
	}
	return nullptr;
}

const TileDefinition* TileDefinition::GetByPixelColor(Rgba8& color)
{
	for (int tileIndex = 0; tileIndex < s_definitions.size(); ++tileIndex)
	{
		TileDefinition* currentDef = s_definitions[tileIndex];
		if (currentDef->m_mapImagePixelColor == color)
		{
			return currentDef;
		}
	}
	return nullptr;
}

Tile::Tile(const TileDefinition* tileDefinition, AABB3 m_bounds)
	: m_tileDefinition(tileDefinition)
	, m_bounds(m_bounds)
{

}
