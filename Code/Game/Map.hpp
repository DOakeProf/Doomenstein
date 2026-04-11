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
class Image;
class Player;
class Actor;

struct Camera;
struct RaycastResult3D;
struct AABB3;
struct AABB2;
struct Vec3;
struct Vertex_PCUTBN;

struct RaycastResultDoomenstein
{
	Vec3 m_rayStartPosition;
	Vec3 m_rayDirection;
	float m_rayLength = 1.f;
	bool m_didImpact = false;
	float m_impactDist = 0.f;
	Vec3 m_impactPos;
	Vec3 m_impactNormal;
	Actor* m_actor = nullptr;
};

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
	Map(Game* game, const MapDefinition* definition, Player* player);
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
	Player* GetCurrentRenderedPlayer();
	std::vector<Actor*> GetActors();

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
	void CollideActorsWithSurroundingTilesXYZ(Actor* actor, Vec3 const& position);
	void CollideActorWithSingleTileXYZ(Actor* actor, Vec3 tilePosition);
	bool PushActorOutOfTileXY(Actor* actor, Tile const& tile);
	void DestroyIfGarbage();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Render
	void Render();
	void Render_Tiles() const;
	void Render_Actors() const;

	RaycastResultDoomenstein RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor* owner = nullptr) const;
	RaycastResult3D RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResultDoomenstein RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor* owner = nullptr) const;

	Game* m_game = nullptr;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Player
	Player* m_player;
	Player* m_currentlyRenderedPlayer;
	Vec3* m_playerTranslationThisFrame;
	bool m_isControllingBullet = false;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Debug
	void Debug_KillAllActors();

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
	Vec3 m_sunDirection;
	float m_sunIntensity;
	float m_ambientIntensity;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Lists of owned objects
	std::vector<Tile> m_tiles;
	std::vector<Actor*> m_actors;
	unsigned int m_nextActorUID = 0;
	Actor* m_bulletActor;

};