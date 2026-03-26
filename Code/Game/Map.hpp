#pragma once
#include <vector>

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Shader.hpp"
#include "Engine/VertexBuffer.hpp"
#include "Engine/IndexBuffer.hpp"
#include "Engine/Core/Image.hpp"

#include "Game/Tile.hpp"

class Game;
class Actor;
class Image;

struct RaycastResult3D;
struct AABB3;
struct AABB2;
struct Vec3;
struct Vertex_PCUTBN;

struct MapDefinition
{
	std::string m_name;
	Image* m_mapImage;
	Shader* m_shader;
	Texture* m_spriteSheetTexture;
	IntVec2 m_spriteSheetCellCount;

	static void InitializeDefinitions(const char* path);
	static void ClearDefinitions();
	static const MapDefinition* GetByName(const std::string& name);
	static std::vector<MapDefinition*> s_definitions;
};

class Map
{
public:
	Map(Game* game, const MapDefinition* definition);
	~Map();

	void CreateTiles();
	void CreateGeometry();
	void AddGeometryForWall(const AABB3& bounds, const AABB2& UVs);
	void AddGeometryForFloor(const AABB3& bounds, const AABB2& UVs);
	void AddGeometryForCeiling(const AABB3& bounds, const AABB2& UVs);
	void CreateBuffers();

	bool IsPositionInBounds(const Vec3& position) const;
	bool AreCoordsInBounds(int x, int y) const;
	const Tile* GetTile(int x, int y) const;

	void Update();
	void CollideActors();
	void CollideActors(Actor* actorA, Actor* actorB);
	void CollideActorsWithMap();
	void CollideActorsWithMap(Actor* actor);

	void Render();

	RaycastResult3D RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor* owner = nullptr) const;
	RaycastResult3D RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor* owner = nullptr) const;

	Game* m_game = nullptr;

protected:

	const MapDefinition* m_definition = nullptr;
	std::vector<Tile> m_tiles;
	IntVec2 m_dimensions;
	
	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indexes;
	const Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;
};