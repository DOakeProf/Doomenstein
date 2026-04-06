#pragma once
#include <vector>

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Shader.hpp"
#include "Engine/VertexBuffer.hpp"
#include "Engine/IndexBuffer.hpp"
#include "Engine/Core/Image.hpp"

#include "Game/Tile.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/SpawnInfo.hpp"

class Game;
class Actor;
class Image;
class Player;

struct Camera;
struct RaycastResult3D;
struct AABB3;
struct AABB2;
struct Vec3;
struct Vertex_PCUTBN;

struct MapDefinition
{
	std::string m_name;
	std::vector<Image*> m_mapImages;
	std::vector<SpawnInfo> m_spawnInfos;
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

	void Startup();
	void Startup_InitializeActors();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Creating Geometry/Map tiles
	void CreateTiles();
	void CreateGeometry();
	void AddGeometryForWall(const AABB3& bounds, const AABB2& UVs);
	void AddGeometryForFloor(const AABB3& bounds, const AABB2& UVs);
	void AddGeometryForCeiling(const AABB3& bounds, const AABB2& UVs);
	void CreateBuffers();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Getting information
	bool IsPositionInBounds(const Vec3& position) const;
	bool AreCoordsInBounds(int x, int y) const;
	const Tile* GetTile(int x, int y) const;
	int GetTileIndexFromWorldPosition(Vec3 position) const;
	int GetTileIndexFromCoordinates(IntVec3 coordinates) const;
	Actor* GetActorByHandle(const ActorHandle handle) const;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Utility functions
	int AddActorToMap(Actor* actor);
	Actor* SpawnActor(const SpawnInfo& spawnInfo);

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Update
	void Update();
	bool Update_KeyboardInput();
	bool Update_KeyboardInputBullet();
	bool Update_ControllerInput();
	void Update_DebugInput();
	void Update_AddDebugScreenText();
	void Update_Actors();
	void CollideActors();
	void CollideActors(Actor* actorA, Actor* actorB);
	void CollideActorsWithMap();
	void CollideActorsWithSurroundingTilesXY(Actor* actor, Vec3 const& position);
	void CollideActorWithSingleTileXY(Actor* actor, Vec3 tilePosition);
	void PushActorOutOfTileXY(Actor* actor, Tile const& tile);
	void CollideActorsWithSurroundingCeilingsAndFloorsXY(Actor* actor);

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Render
	void Render() const;
	void Render_Tiles() const;
	void Render_Actors() const;

	RaycastResult3D RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor* owner = nullptr) const;
	RaycastResult3D RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor* owner = nullptr) const;

	Game* m_game = nullptr;

protected:

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Properties
	const MapDefinition* m_definition = nullptr;
	IntVec3 m_dimensions;
	SpriteSheet m_tileSpriteSheet;
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Graphics rendering
	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indexes;
	const Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;
	Camera* m_screenCamera = nullptr;
	Vec3 m_sunDirection;
	float m_sunIntensity;
	float m_ambientIntensity;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Lists of owned objects
	std::vector<Tile> m_tiles;
	std::vector<Actor*> m_actors;
	unsigned int m_nextActorUID = 0;
	Actor* m_bulletActor;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Player
	Player* m_player;
	Vec3* m_playerTranslationThisFrame;
	bool m_isControllingBullet = false;
};