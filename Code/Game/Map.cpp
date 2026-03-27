#include "Game/Map.hpp"

#include "Game/Game.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/XmlUtils.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/VertexUtils.hpp"

std::vector<MapDefinition*> MapDefinition::s_definitions;

Map::Map(Game* game, const MapDefinition* definition)
	:m_game(game)
	, m_definition(definition)
{
	m_tileSpriteSheet = SpriteSheet(m_definition->m_spriteSheetTexture, m_definition->m_spriteSheetCellCount);
	CreateTiles();
	CreateGeometry();
}

Map::~Map()
{

}

void Map::CreateTiles()
{
	IntVec2 defDimensions = m_definition->m_mapImage->GetDimensions();
	for (int xCoord = 0; xCoord < defDimensions.x; ++xCoord)
	{
		for (int yCoord = 0; yCoord < defDimensions.y; ++yCoord)
		{
			Rgba8 texelColor = m_definition->m_mapImage->GetTexelColor(IntVec2(xCoord, yCoord));
			const TileDefinition* tileDef = TileDefinition::GetByPixelColor(texelColor);
			AABB3 tileAABB3 = AABB3(Vec3((float)xCoord, (float)yCoord, 0.f), Vec3((float)xCoord + 1.f, (float)yCoord + 1.f, 1.f));
 			m_tiles.emplace_back(tileDef, tileAABB3);
		}
	}
}

void Map::CreateGeometry()
{
	for (int tileIndex = 0; tileIndex < m_tiles.size(); ++tileIndex)
	{
		Tile currentTile = m_tiles[tileIndex];
		const TileDefinition* currentTileDefinition = currentTile.m_tileDefinition;
		AABB3 currentBounds = currentTile.m_bounds;
		
		if (currentTileDefinition->m_floorSpriteCoords != IntVec2(-1, -1))
		{
			AABB2 currentUVs = m_tileSpriteSheet.GetUVsForSprite(currentTileDefinition->m_floorSpriteCoords);
			AddGeometryForFloor(currentBounds, currentUVs);
		}
		if (currentTileDefinition->m_wallSpriteCoords != IntVec2(-1, -1))
		{
			AABB2 currentUVs = m_tileSpriteSheet.GetUVsForSprite(currentTileDefinition->m_wallSpriteCoords);
			AddGeometryForWall(currentBounds, currentUVs);
		}
		if (currentTileDefinition->m_ceilingSpriteCoords != IntVec2(-1, -1))
		{
			AABB2 currentUVs = m_tileSpriteSheet.GetUVsForSprite(currentTileDefinition->m_ceilingSpriteCoords);
			AddGeometryForCeiling(currentBounds, currentUVs);
		}
	}
}

void Map::AddGeometryForWall(const AABB3& bounds, const AABB2& UVs)
{
	// + X
	AddVertsForQuad3D(m_vertexes, m_indexes,
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_mins.z),
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_mins.z),
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_maxs.z),
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_maxs.z),
		Rgba8::WHITE,
		UVs);
	// + Y
	AddVertsForQuad3D(m_vertexes, m_indexes,
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_mins.z),
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_mins.z),
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_maxs.z),
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_maxs.z),
		Rgba8::WHITE,
		UVs);
	// - X
	AddVertsForQuad3D(m_vertexes, m_indexes,
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_mins.z),
		Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_mins.z),
		Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.z),
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_maxs.z),
		Rgba8::WHITE,
		UVs);
	// - Y
	AddVertsForQuad3D(m_vertexes, m_indexes,
		Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_mins.z),
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_mins.z),
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_maxs.z),
		Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.z),
		Rgba8::WHITE,
		UVs);
}

void Map::AddGeometryForFloor(const AABB3& bounds, const AABB2& UVs)
{
	AddVertsForQuad3D(m_vertexes, m_indexes, 
		Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_mins.z),
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_mins.z),
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_mins.z),
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_mins.z),
		Rgba8::WHITE,
		UVs);
}

void Map::AddGeometryForCeiling(const AABB3& bounds, const AABB2& UVs)
{
	AddVertsForQuad3D(m_vertexes, m_indexes,
		Vec3(bounds.m_mins.x, bounds.m_maxs.y, bounds.m_maxs.z),
		Vec3(bounds.m_maxs.x, bounds.m_maxs.y, bounds.m_maxs.z),
		Vec3(bounds.m_maxs.x, bounds.m_mins.y, bounds.m_maxs.z),
		Vec3(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.z),
		Rgba8::WHITE,
		UVs);
}

void Map::CreateBuffers()
{

}

bool Map::IsPositionInBounds(const Vec3& position) const
{
	return false;
}

bool Map::AreCoordsInBounds(int x, int y) const
{
	return false;
}

const Tile* Map::GetTile(int x, int y) const
{
	return &m_tiles[0];
}

void Map::CollideActors()
{

}

void Map::CollideActors(Actor* actorA, Actor* actorB)
{

}

void Map::CollideActorsWithMap()
{

}

void Map::CollideActorsWithMap(Actor* actor)
{

}

void Map::Render()
{
	g_engine->m_render->BindTexture(m_tileSpriteSheet.GetTexture());
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	//g_engine->m_render->BindShader(m_definition->m_shader);
	g_engine->m_render->DrawIndexedVertexList(&m_vertexes, &m_indexes);
}

RaycastResult3D Map::RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor* owner /*= nullptr*/) const
{
	return RaycastResult3D();
}

RaycastResult3D Map::RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const
{
	return RaycastResult3D();
}

RaycastResult3D Map::RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const
{
	return RaycastResult3D();
}

RaycastResult3D Map::RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor* owner /*= nullptr*/) const
{
	return RaycastResult3D();
}

void MapDefinition::InitializeDefinitions(const char* path)
{
	XmlDocument tileDefsXml;
	[[maybe_unused]] XmlResult result = tileDefsXml.LoadFile(path);
	XmlElement* rootElement = tileDefsXml.RootElement();
	XmlElement* mapDefElement = rootElement->FirstChildElement();
	while (mapDefElement)
	{
		MapDefinition* newMapDef = new MapDefinition();
		newMapDef->m_name = ParseXmlAttribute(*mapDefElement, "name", "");
		std::string mapImagePath = ParseXmlAttribute(*mapDefElement, "image", "");
		newMapDef->m_mapImage = new Image(mapImagePath.data());
		std::string shaderPath = ParseXmlAttribute(*mapDefElement, "shader", "");
		newMapDef->m_shader = g_engine->m_render->CreateShader(shaderPath.data(), VertexType::VERTEX_PCUTBN);
		std::string texturePath = ParseXmlAttribute(*mapDefElement, "spriteSheetTexture", "");
		newMapDef->m_spriteSheetTexture = g_engine->m_render->CreateOrGetTextureFromFile(texturePath.data());
		newMapDef->m_spriteSheetCellCount = ParseXmlAttribute(*mapDefElement, "spriteSheetCellCount", IntVec2(-1, -1));
		s_definitions.push_back(newMapDef);
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}

void MapDefinition::ClearDefinitions()
{
	s_definitions.clear();
}

const MapDefinition* MapDefinition::GetByName(const std::string& name)
{
	for (int mapIndex = 0; mapIndex < s_definitions.size(); ++mapIndex)
	{
		MapDefinition* currentDef = s_definitions[mapIndex];
		if (currentDef->m_name == name)
		{
			return currentDef;
		}
	}
}
