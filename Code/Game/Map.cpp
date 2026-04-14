#include "Game/Map.hpp"

#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/AI.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/XmlUtils.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"

std::vector<MapDefinition*> MapDefinition::s_definitions;

Map::Map(Game* game, const MapDefinition* definition, Player* player)
	:m_game(game)
	, m_definition(definition)
	, m_player(player)
{
	m_player->m_map = this;
	Startup();
}

Map::~Map()
{
	delete m_vertexBuffer;
	delete m_indexBuffer;
	delete m_clipPlaneCBO;
	delete m_portalAABB3CBO;

	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	m_clipPlaneCBO = nullptr;
	m_portalAABB3CBO = nullptr;
}

void Map::Startup()
{
	m_tileSpriteSheet = SpriteSheet(m_definition->m_spriteSheetTexture, m_definition->m_spriteSheetCellCount);
	IntVec2 dimensionsXY = m_definition->m_mapImages[0]->GetDimensions();
	m_dimensions = IntVec3(dimensionsXY.x, dimensionsXY.y, (int)m_definition->m_mapImages.size());
	CreateTiles();
	CreateGeometry();
	CreateBuffers();

	m_sunDirection = Vec3(2.f, 1.f, -1.f);
	m_sunIntensity = 0.85f;
	m_ambientIntensity = 0.35f;

	m_clipPlaneCBO = new ConstantBuffer(g_engine->m_render->GetDevice(), sizeof(ClipPlaneConstants));
	m_portalAABB3CBO = new ConstantBuffer(g_engine->m_render->GetDevice(), sizeof(PortalAABB3Constants));
}

void Map::Startup_InitializeActors()
{
	for (int spawnInfoIndex = 0; spawnInfoIndex < m_definition->m_spawnInfos.size(); ++spawnInfoIndex)
	{
		SpawnActor(m_definition->m_spawnInfos[spawnInfoIndex]);
	}
}

void Map::CreateTiles()
{
	for (int zCoord = 0; zCoord < m_definition->m_mapImages.size(); ++zCoord)
	{
		for (int xCoord = 0; xCoord < m_dimensions.x; ++xCoord)
		{
			for (int yCoord = 0; yCoord < m_dimensions.y; ++yCoord)
			{
				Rgba8 texelColor = m_definition->m_mapImages[zCoord]->GetTexelColor(IntVec2(xCoord, yCoord));
				const TileDefinition* tileDef = TileDefinition::GetByPixelColor(texelColor);
				AABB3 tileAABB3 = AABB3(Vec3((float)xCoord, (float)yCoord, (float)zCoord), Vec3((float)xCoord + 1.f, (float)yCoord + 1.f, (float)zCoord + 1.f));
				m_tiles.emplace_back(tileDef, tileAABB3);
			}
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
	m_vertexBuffer = g_engine->m_render->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_engine->m_render->CreateIndexBuffer(sizeof(unsigned int));
}

bool Map::IsPositionInBounds(const Vec3& position) const
{
	AABB2 mapBounds = AABB2(Vec2(0.f, 0.f), Vec2((float)m_dimensions.x, (float)m_dimensions.y));
	if (mapBounds.IsPointInside(Vec2(position.x, position.y)))
	{
		return true;
	}
	return false;
}

bool Map::AreCoordsInBounds(int x, int y) const
{
	AABB2 mapBounds = AABB2(Vec2(0.f, 0.f), Vec2((float)m_dimensions.x, (float)m_dimensions.y));
	if (mapBounds.IsPointInside(Vec2((float)x, (float)y)))
	{
		return true;
	}
	return false;
}

const Tile* Map::GetTile(int x, int y) const
{
	if (x < 0 ||
		y < 0 ||
		x > x - 1 ||
		y > y - 1)
	{
		return nullptr;
	}
	int indexToReturn = (x * y) + y;
	if (indexToReturn < 0 || indexToReturn > m_tiles.size() - 1)
	{
		return nullptr;
	}
	return &m_tiles[indexToReturn];
}

const Tile* Map::GetTile(int index) const
{
	if (index < 0 || index > m_dimensions.x * m_dimensions.y * m_definition->m_mapImages.size())
	{
		return nullptr;
	}
	return &m_tiles[index];
}

int Map::GetTileIndexFromWorldPosition(Vec3 position) const
{
	if (position.x < 0.f || 
		position.y < 0.f ||
		position.x > (float)m_dimensions.x ||
		position.y > (float)m_dimensions.y)
	{
		return -1;
	}
	int indexToReturn = (RoundDownToInt(position.x) * m_dimensions.y) + RoundDownToInt(position.y) + (RoundDownToInt(position.z) * m_dimensions.x * m_dimensions.y);
	if (indexToReturn < 0 || indexToReturn > m_tiles.size() - 1)
	{
		return -1;
	}
	return indexToReturn;
}

int Map::GetTileIndexFromCoordinates(IntVec3 coordinates) const
{
	if (coordinates.x < 0 || 
		coordinates.y < 0 || 
		coordinates.z < 0 ||
		coordinates.x > m_dimensions.x - 1 || 
		coordinates.y > m_dimensions.y - 1 ||
		coordinates.z > m_dimensions.z - 1)
	{
		return -1;
	}
	int indexToReturn = ((coordinates.x * m_dimensions.y) + coordinates.y) + (m_dimensions.x * m_dimensions.y * coordinates.z);
	if (indexToReturn < 0 || indexToReturn > m_tiles.size() - 1)
	{
		return -1;
	}
	return indexToReturn;
}

Actor* Map::GetActorByHandle(const ActorHandle handle) const
{
	int actorIndex = handle.GetIndex();
	Actor* actor = m_actors[actorIndex];
	if (actor != nullptr && handle.GetData() == actor->m_handle->GetData())
	{
		return actor;
	}
	return nullptr;
}

Player* Map::GetCurrentRenderedPlayer()
{
	return m_currentlyRenderedPlayer;
}

std::vector<Actor*> Map::GetActors()
{
	return m_actors;
}

int Map::AddActorToMap(Actor* actor)
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* currentActor = m_actors[actorIndex];
		if (currentActor == nullptr)
		{
			m_actors[actorIndex] = actor;
			return actorIndex;
		}
	}
	m_actors.push_back(actor);

	return (int)m_actors.size() - 1;
}

Actor* Map::SpawnActor(const SpawnInfo& spawnInfo)
{
	Actor* newActor = new Actor(this, spawnInfo.m_name, spawnInfo.m_position, spawnInfo.m_orientation);
	int actorIndex = AddActorToMap(newActor);
	ActorHandle* actorHandle = new ActorHandle(m_nextActorUID, actorIndex);
	newActor->m_handle = actorHandle;
	++m_nextActorUID;
	if (m_nextActorUID > ActorHandle::MAX_ACTOR_UID)
	{
		m_nextActorUID = 0;
	}

	if (newActor->m_definition->m_name == "Marine")
	{
		m_player->Possess(actorHandle);
	}
	if (newActor->m_definition->m_aiEnabled)
	{
		AI* aiController = new AI(this);
		aiController->Possess(actorHandle);
	}

	return newActor;
}

void Map::AddPortal(Portal* portal)
{
	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portalToCheck = m_portals[portalIndex];
		if (portalToCheck == nullptr)
		{
			m_portals[portalIndex] = portal;
			return;
		}
	}
	m_portals.push_back(portal);
}

void Map::RemovePortal(Portal* portal)
{
	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portalToCheck = m_portals[portalIndex];
		if (portal == portalToCheck)
		{
			delete portalToCheck;
			m_portals[portalIndex] = nullptr;
			return;
		}
	}
	ERROR_AND_DIE("Attempted to remove portal that wasn't in map.");
}

void Map::Update()
{
	XboxController* controller = &g_engine->m_input->m_controllers[0];
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC) || controller->WasButtonJustPressed(XboxButtonID::BACK))
	{
		m_game->ChangeGameState(GameState::GAME_STATE_ATTRACT);
		return;
	}

	if (g_engine->m_input->WasKeyJustPressed('K'))
	{
		Debug_KillAllActors();
	}

	Update_AddDebugScreenText();

	Update_DebugInput();

	Update_Actors_BeforePreventative(); // Assigns each actors a desired position
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Preventative physics
	// Modifies desired position based on preventative checks.
	CollideActorsWithPortals();

	Update_Actors_AfterPreventative(); // Updates the actual position of each actor to be the desired position.

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Corrective Physics
	CollideActors();
	CollideActorsWithMap();

	Update_Portals();

	DestroyIfGarbage();
}

bool Map::Update_KeyboardInput()
{
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		m_game->ChangeGameState(GameState::GAME_STATE_ATTRACT);
		return true;
	}

	if (g_engine->m_input->IsKeyDown('T'))
	{
		m_game->m_gameClock->SetTimeScale(0.1f);
	}
	else
	{
		m_game->m_gameClock->SetTimeScale(1.f);
	}

	float currentMoveSpeed = m_game->m_moveSpeed;
	if (g_engine->m_input->IsKeyDown(KEYCODE_SHIFT))
	{
		currentMoveSpeed *= 15.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_player->m_orientation;
	newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	if (g_engine->m_input->IsKeyDown('Q'))
	{
		newOrientation.m_rollDegrees -= (float)s_systemClock->GetDeltaSeconds() * m_game->m_rollSensitivity;
	}
	if (g_engine->m_input->IsKeyDown('E'))
	{
		newOrientation.m_rollDegrees += (float)s_systemClock->GetDeltaSeconds() * m_game->m_rollSensitivity;
	}
	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_player->m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_player->m_position;
	Vec3 newAddedPosition;
	if (g_engine->m_input->IsKeyDown('A'))
	{
		newAddedPosition.y += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('D'))
	{
		newAddedPosition.y -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('W'))
	{
		newAddedPosition.x += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('S'))
	{
		newAddedPosition.x -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	Mat44 orientationMatrix = newOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	orientationMatrix.AppendTranslation3D(newAddedPosition);
	Vec3 newTranslation = orientationMatrix.GetTranslation3D();

	if (g_engine->m_input->IsKeyDown('Z'))
	{
		newTranslation.z -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('C'))
	{
		newTranslation.z += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}

	m_playerTranslationThisFrame->x = newTranslation.x;
	m_playerTranslationThisFrame->y = newTranslation.y;
	m_playerTranslationThisFrame->z = newTranslation.z;
	m_player->m_position += newTranslation;

	if (g_engine->m_input->IsKeyDown('H'))
	{
		m_player->m_position = Vec3();
		m_player->m_orientation = EulerAngles();
	}

	if (g_engine->m_input->WasKeyJustPressed('P'))
	{
		m_game->m_gameClock->TogglePause();
	}

	if (g_engine->m_input->WasKeyJustPressed('O'))
	{
		m_game->m_gameClock->StepSingleFrame();
	}

	DebugAddMessage("[F1] Control Mode: Player", 0.f);

	return false;
}

bool Map::Update_KeyboardInputBullet()
{


	if (g_engine->m_input->IsKeyDown('T'))
	{
		m_game->m_gameClock->SetTimeScale(0.1f);
	}
	else
	{
		m_game->m_gameClock->SetTimeScale(1.f);
	}

	float currentMoveSpeed = m_game->m_moveSpeed;
	if (g_engine->m_input->IsKeyDown(KEYCODE_SHIFT))
	{
		currentMoveSpeed *= 15.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_player->m_orientation;
	newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	if (g_engine->m_input->IsKeyDown('Q'))
	{
		newOrientation.m_rollDegrees -= (float)s_systemClock->GetDeltaSeconds() * m_game->m_rollSensitivity;
	}
	if (g_engine->m_input->IsKeyDown('E'))
	{
		newOrientation.m_rollDegrees += (float)s_systemClock->GetDeltaSeconds() * m_game->m_rollSensitivity;
	}
	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_player->m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_bulletActor->m_position;
	Vec3 newAddedPosition;
	if (g_engine->m_input->IsKeyDown('A'))
	{
		newAddedPosition.y += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('D'))
	{
		newAddedPosition.y -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('W'))
	{
		newAddedPosition.x += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('S'))
	{
		newAddedPosition.x -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	Vec3 orientationFwd = newOrientation.GetForwardDir_IFwd_JLeft_KUp();
	Vec3 orientationFwdXY = Vec3(orientationFwd.x, orientationFwd.y, 0.f).GetNormalized();
	Mat44 orientationMatrix = Mat44(orientationFwdXY, orientationFwdXY.GetRotatedAboutZDegrees(90.f), Vec3(0.f, 0.f, 1.f), Vec3());
	orientationMatrix.AppendTranslation3D(newAddedPosition);
	Vec3 newTranslation = orientationMatrix.GetTranslation3D();

	if (g_engine->m_input->IsKeyDown('Z'))
	{
		newTranslation.z -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('C'))
	{
		newTranslation.z += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}

	m_bulletActor->m_position += newTranslation;

	if (g_engine->m_input->IsKeyDown('H'))
	{
		m_bulletActor->m_position = Vec3();
		m_bulletActor->m_orientation = EulerAngles();
	}

	if (g_engine->m_input->WasKeyJustPressed('P'))
	{
		m_game->m_gameClock->TogglePause();
	}

	if (g_engine->m_input->WasKeyJustPressed('O'))
	{
		m_game->m_gameClock->StepSingleFrame();
	}

	DebugAddMessage("[F1] Control Mode: Actor", 0.f, Rgba8::BLUE);

	return false;
}

bool Map::Update_ControllerInput()
{
	XboxController* controller = &g_engine->m_input->m_controllers[0];
	if (controller->WasButtonJustPressed(XboxButtonID::BACK))
	{
		m_game->ChangeGameState(GameState::GAME_STATE_ATTRACT);
		return true;
	}

	float currentMoveSpeed = m_game->m_moveSpeed;
	if (controller->IsButtonDown(XboxButtonID::GAMEPAD_A))
	{
		currentMoveSpeed *= 15.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_player->m_orientation;
	newOrientation.m_yawDegrees -= controller->GetRightStick().GetPosition().x * m_game->m_controllerSensitivity;
	newOrientation.m_pitchDegrees -= controller->GetRightStick().GetPosition().y * m_game->m_controllerSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);

	newOrientation.m_rollDegrees += controller->GetRightTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_game->m_rollSensitivity;
	newOrientation.m_rollDegrees -= controller->GetLeftTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_game->m_rollSensitivity;

	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_player->m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_player->m_position;
	Vec3 newAddedPosition;
	newAddedPosition.y -= controller->GetLeftStick().GetPosition().x * (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	newAddedPosition.x += controller->GetLeftStick().GetPosition().y * (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;

	Mat44 orientationMatrix = newOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	orientationMatrix.AppendTranslation3D(newAddedPosition);
	Vec3 newTranslation = orientationMatrix.GetTranslation3D();

	if (controller->IsButtonDown(XboxButtonID::LEFT_SHOULDER))
	{
		newTranslation.z -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (controller->IsButtonDown(XboxButtonID::RIGHT_SHOULDER))
	{
		newTranslation.z += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}

	if (newTranslation.x != 0.f)
	{
		m_playerTranslationThisFrame->x = newTranslation.x;
	}
	if (newTranslation.y != 0.f)
	{
		m_playerTranslationThisFrame->y = newTranslation.y;
	}
	if (newTranslation.z != 0.f)
	{
		m_playerTranslationThisFrame->z = newTranslation.z;
	}

	if (controller->IsButtonDown(XboxButtonID::START))
	{
		m_player->m_position = Vec3();
		m_player->m_orientation = EulerAngles();
	}

	return false;
}

void Map::Update_DebugInput()
{
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F1))
	{
		m_isControllingBullet = !m_isControllingBullet;
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F2))
	{
		m_sunDirection.x -= 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F3))
	{
		m_sunDirection.x += 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F4))
	{
		m_sunDirection.y -= 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F5))
	{
		m_sunDirection.y += 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_sunDirection.x, m_sunDirection.y, m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F6))
	{
		m_sunIntensity -= 0.05f;
		m_sunIntensity = GetClamped(m_sunIntensity, 0.f, 1.f);
		std::string message = Stringf("Sun Intensity: %.2f", m_sunIntensity);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F7))
	{
		m_sunIntensity += 0.05f;
		m_sunIntensity = GetClamped(m_sunIntensity, 0.f, 1.f);
		std::string message = Stringf("Sun Intensity: %.2f", m_sunIntensity);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F8))
	{
		m_ambientIntensity -= 0.05f;
		m_ambientIntensity = GetClamped(m_ambientIntensity, 0.f, 1.f);
		std::string message = Stringf("Ambient Intensity: %.2f", m_ambientIntensity);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F9))
	{
		m_ambientIntensity += 0.05f;
		m_ambientIntensity = GetClamped(m_ambientIntensity, 0.f, 1.f);
		std::string message = Stringf("Ambient Intensity: %.2f", m_ambientIntensity);
		DebugAddMessage(message, 2.f);
	}

	//if (g_engine->m_input->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	//{
	//	RaycastAll(m_player->m_position, m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), 10.f, nullptr);
	//}
	//if (g_engine->m_input->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	//{
	//	RaycastAll(m_player->m_position, m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), 0.25f, nullptr);
	//}
}

void Map::Update_AddDebugScreenText()
{
	std::string clockText = Stringf("[GAME CLOCK] Time: %.2f FPS: %.2f Time Scale: %.2f", m_game->m_gameClock->GetTotalSeconds(), m_game->m_gameClock->GetFrameRate(), m_game->m_gameClock->GetTimeScale());
	DebugAddScreenText(clockText, AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), SCREEN_SIZE_Y * 0.02f, Vec2(0.98f, 0.97f), 0.f);

	std::string lightingText = Stringf("Sun Direction X: %.2f [F2 / F3 to change]\nSun Direction Y: %.2f [F4 / F5 to change]\nSun Intensity: %.2f [F6 / F7 to change]\nAmbient Intensity: %.2f [F8 / F9 to change]", m_sunDirection.x, m_sunDirection.y, m_sunIntensity, m_ambientIntensity);
	DebugAddScreenText(lightingText, AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), SCREEN_SIZE_Y * 0.02f, Vec2(0.98f, 0.94f), 0.f);
}

void Map::Update_Actors_BeforePreventative()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* actor = m_actors[actorIndex];
		if (actor != nullptr)
		{
			m_actors[actorIndex]->Update();
		}
	}
}

void Map::Update_Actors_AfterPreventative()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* actor = m_actors[actorIndex];
		if (actor != nullptr)
		{
			m_actors[actorIndex]->Update_Position();
		}
	}
}

void Map::Update_Portals()
{
	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portal = m_portals[portalIndex];
		if (portal != nullptr)
		{
			portal->Update();
		}
	}
}

void Map::CollideActors()
{
	for (int actorAIndex = 0; actorAIndex < m_actors.size(); ++actorAIndex)
	{
		Actor* actorA = m_actors[actorAIndex];
		if (actorA != nullptr && 
			!actorA->m_isDead && 
			actorA->m_definition->m_collidesWithActors
			)
		{
			for (int actorBIndex = actorAIndex + 1; actorBIndex < m_actors.size(); ++actorBIndex)
			{
				Actor* actorB = m_actors[actorBIndex];
				if (actorB != nullptr && 
					!actorB->m_isDead && 
					actorB->m_definition->m_collidesWithActors &&
					actorB->m_owner != actorA
					)
				{
					CollideActors(actorA, actorB);
				}
			}
		}
	}
}

void Map::CollideActors(Actor* actorA, Actor* actorB)
{
	FloatRange actorAZRange = FloatRange(actorA->m_position.z, actorA->m_position.z + actorA->m_definition->m_height);
	FloatRange actorBZRange = FloatRange(actorB->m_position.z, actorB->m_position.z + actorB->m_definition->m_height);

	if (!actorAZRange.IsOverlappingWith(actorBZRange) ||
		!actorA->m_definition->m_collidesWithSameActor && actorA->m_definition == actorB->m_definition)
	{
		return;
	}

	Vec2 actorADisc = Vec2(actorA->m_position.x, actorA->m_position.y);
	Vec2 actorBDisc = Vec2(actorB->m_position.x, actorB->m_position.y);
	bool didActorsCollide = PushDiscsOutOfEachOther2D(actorADisc, actorA->m_definition->m_radius, actorBDisc, actorB->m_definition->m_radius);
	actorA->m_position.x = actorADisc.x;
	actorA->m_position.y = actorADisc.y;
	actorB->m_position.x = actorBDisc.x;
	actorB->m_position.y = actorBDisc.y;

	if (didActorsCollide)
	{
		actorA->OnCollide(actorB);
		actorB->OnCollide(actorA);
	}
}

void Map::CollideActorsWithSurroundingTilesXYZ(Actor* actor, Vec3 const& position)
{
	Vec3 NORTH		= Vec3(0.f, 1.f, 0.f);
	Vec3 NORTH_EAST	= Vec3(1.f, 1.f, 0.f);
	Vec3 EAST		= Vec3(1.f, 0.f, 0.f);
	Vec3 SOUTH_EAST = Vec3(1.f, -1.f, 0.f);
	Vec3 SOUTH		= Vec3(0.f, -1.f, 0.f);
	Vec3 SOUTH_WEST = Vec3(-1.f, -1.f, 0.f);
	Vec3 WEST		= Vec3(-1.f, 0.f, 0.f);
	Vec3 NORTH_WEST = Vec3(-1.f, 1.f, 0.f);

	CollideActorWithSingleTileXYZ(actor, position);
	CollideActorWithSingleTileXYZ(actor, position + NORTH);
	CollideActorWithSingleTileXYZ(actor, position + NORTH_EAST);
	CollideActorWithSingleTileXYZ(actor, position + EAST);
	CollideActorWithSingleTileXYZ(actor, position + SOUTH_EAST);
	CollideActorWithSingleTileXYZ(actor, position + SOUTH);
	CollideActorWithSingleTileXYZ(actor, position + SOUTH_WEST);
	CollideActorWithSingleTileXYZ(actor, position + WEST);
	CollideActorWithSingleTileXYZ(actor, position + NORTH_WEST);
}

void Map::CollideActorWithSingleTileXYZ(Actor* actor, Vec3 tilePosition)
{
	int tileIndex = GetTileIndexFromWorldPosition(tilePosition);
	if (tileIndex == -1)
	{
		return;
	}
	Tile tile = m_tiles[tileIndex];

	bool didCollide = false;
	FloatRange actorZRange = FloatRange(actor->m_position.z, actor->m_position.z + actor->m_definition->m_height);
	FloatRange tilezRange = FloatRange(tile.m_bounds.m_mins.z + 0.1f, tile.m_bounds.m_maxs.z - 0.1f);

	bool doesActorOverlapAABB2D = DoesDiscOverlapAABB2D(Vec2(actor->m_position.x, actor->m_position.y), actor->m_definition->m_radius, AABB2(Vec2(tile.m_bounds.m_mins.x, tile.m_bounds.m_mins.y), Vec2(tile.m_bounds.m_maxs.x, tile.m_bounds.m_maxs.y)));
	bool doesActorOverlapHorizontalPortal = IsActorOverlappingHorizontalPortal(actor, tilezRange);

	//FloatRange tilezRange = FloatRange(-1.f, -1.f);
	if (doesActorOverlapAABB2D &&
		!doesActorOverlapHorizontalPortal
		)
	{
		// Collide with ceiling
		if (tile.m_tileDefinition->m_ceilingSpriteCoords != IntVec2(-1, -1) &&
			actorZRange.IsOnRange(tile.m_bounds.m_maxs.z))
		{
			actor->m_position.z = tile.m_bounds.m_maxs.z - actor->m_definition->m_height;
			didCollide = true;
			//actor->m_velocity.z = 0.f;
		}
		// Collide with floor
		if (tile.m_tileDefinition->m_floorSpriteCoords != IntVec2(-1, -1) &&
			actorZRange.IsOnRange(tile.m_bounds.m_mins.z))
		{
			actor->m_position.z = tile.m_bounds.m_mins.z;
			actor->m_isGrounded = true;
			actor->m_isJumping = false;
			didCollide = true;
		}
	}

	bool doesActorOverlapVerticalPortal = IsActorOverlappingVerticalPortal(actor);
	if (tile.m_tileDefinition->m_isSolid && 
		actorZRange.IsOverlappingWith(tilezRange) &&
		!doesActorOverlapVerticalPortal)
	{
		didCollide = PushActorOutOfTileXY(actor, tile);
	}

	if (doesActorOverlapVerticalPortal)
	{
		int i = 0;
	}

	if (didCollide)
	{
		actor->OnCollide(nullptr);
	}
}

bool Map::IsActorOverlappingHorizontalPortal(Actor* actor, FloatRange& tileZRange)
{
	bool doesActorOverlapPortal = false;
	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portal = m_portals[portalIndex];
		// Is the portal vertical and in our tile's Z range?
		EulerAngles portalOrientation = portal->GetOrientation();
		Vec3 portalForward = portalOrientation.GetForwardDir_IFwd_JLeft_KUp();
		float portalPosInMiddleOfTile = (float)RoundDownToInt(portal->GetPosition().z) + 0.5f;
		if (portalForward.z != 0.f &&
			tileZRange.IsOnRange(portalPosInMiddleOfTile)
			)
		{
			Vec3 portalJBasis = portalOrientation.GetLeftDir_IFwd_JLeft_KUp();
			if (
				IsDiscInsideOBB2D(
					Vec2(actor->m_position.x, actor->m_position.y)
					, actor->m_definition->m_radius
					, OBB2(Vec2(portal->GetPosition().x, portal->GetPosition().y), Vec2(portalJBasis.x, portalJBasis.y), Vec2(portal->tl.y, portal->tl.z))
				)
				) // Is the actor over/under the portal
			{
				doesActorOverlapPortal = true;
			}
		}
	}
	return doesActorOverlapPortal;
}

bool Map::IsActorOverlappingVerticalPortal(Actor* actor)
{
	bool doesActorOverlapPortal = false;
	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portal = m_portals[portalIndex];
		EulerAngles portalOrientation = portal->GetOrientation();
		// Portal along the X axis
		if (portalOrientation.GetForwardDir_IFwd_JLeft_KUp().x == 0.f)
		{
			// Do an AABB2 check with the portal's X and Z and the actor's X and Z. project the actor onto a flat AABB2.
			AABB2 playerXZAABB = AABB2(
				Vec2(actor->m_position.x - actor->m_definition->m_radius, actor->m_position.z + 0.001f)
				, Vec2(actor->m_position.x + actor->m_definition->m_radius, actor->m_position.z + actor->m_definition->m_height)
			);
			AABB2 portalXZAABB = AABB2(
				Vec2(portal->bl.x + portal->GetPosition().x, portal->bl.z + portal->GetPosition().z)
				, Vec2(portal->tr.x + portal->GetPosition().x, portal->tr.z + portal->GetPosition().z)
			);

			if (portalXZAABB.m_mins.x > portalXZAABB.m_maxs.x)
			{
				float xStorage = portalXZAABB.m_mins.x;
				portalXZAABB.m_mins.x = portalXZAABB.m_maxs.x;
				portalXZAABB.m_maxs.x = xStorage;
			}
			if (portalXZAABB.m_mins.y > portalXZAABB.m_maxs.y)
			{
				float yStorage = portalXZAABB.m_mins.y;
				portalXZAABB.m_mins.y = portalXZAABB.m_maxs.y;
				portalXZAABB.m_maxs.y = yStorage;
			}

			FloatRange actorYRange = FloatRange(actor->m_position.y - actor->m_definition->m_radius, actor->m_position.y + actor->m_definition->m_radius);

			if (ISAABB2InsideAABB2(playerXZAABB, portalXZAABB) &&
				actorYRange.IsOnRange(portal->GetPosition().y))
			{
				return true;
			}
		}
		// Portal is along the Y axis
		else
		{
			// Do an AABB2 check with the portal's Y and Z and the actor's Y and Z. project the actor onto a flat AABB2.
			AABB2 playerXZAABB = AABB2(
				Vec2(actor->m_position.y - actor->m_definition->m_radius, actor->m_position.z + 0.001f)
				, Vec2(actor->m_position.y + actor->m_definition->m_radius, actor->m_position.z + actor->m_definition->m_height)
			);
			AABB2 portalXZAABB = AABB2(
				Vec2(portal->bl.y + portal->GetPosition().y, portal->bl.z + portal->GetPosition().z)
				, Vec2(portal->tr.y + portal->GetPosition().y, portal->tr.z + portal->GetPosition().z)
			);

			if (portalXZAABB.m_mins.x > portalXZAABB.m_maxs.x)
			{
				float xStorage = portalXZAABB.m_mins.x;
				portalXZAABB.m_mins.x = portalXZAABB.m_maxs.x;
				portalXZAABB.m_maxs.x = xStorage;
			}
			if (portalXZAABB.m_mins.y > portalXZAABB.m_maxs.y)
			{
				float yStorage = portalXZAABB.m_mins.y;
				portalXZAABB.m_mins.y = portalXZAABB.m_maxs.y;
				portalXZAABB.m_maxs.y = yStorage;
			}

			FloatRange actorXRange = FloatRange(actor->m_position.x - actor->m_definition->m_radius, actor->m_position.x + actor->m_definition->m_radius);

			if (ISAABB2InsideAABB2(playerXZAABB, portalXZAABB) &&
				actorXRange.IsOnRange(portal->GetPosition().x))
			{
				return true;
			}
		}
	}

	return doesActorOverlapPortal;
}

void Map::CollideActorsWithPortals()
{
	// Maybe do a sphere vs sphere check first to see if an actor is close enough with a portal, and if so do a ray cast?
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* actor = m_actors[actorIndex];
		if (actor != nullptr)
		{
			for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
			{
				Portal* portal = m_portals[portalIndex];
				if (portal != nullptr && portal->GetOtherPortal() != nullptr)
				{
					bool didCollideWithPortal = CollideActorWithPortal(actor, portal);
					if (didCollideWithPortal)
					{
						break; // Makes it so that the actor can only collide with ONE portal per frame.
					}
				}
			}
		}
	}
}

bool Map::CollideActorWithPortal(Actor* actor, Portal* portal)
{
	Vec3 actorEyePos = actor->m_position + Vec3(0.f,0.f, actor->m_definition->m_eyeHeight);
	Vec3 actorDesiredEyePos = actor->m_desiredPosition + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);

	Vec3 rayStart = actorEyePos;
	Vec3 actorTranslationThisFrame = actorDesiredEyePos - actorEyePos;
	Vec3 rayFwdNormal = actorTranslationThisFrame.GetNormalized();
	float rayLength = actorTranslationThisFrame.GetLength();

	Vec3 portalToActor = actorEyePos - portal->GetPosition();
	Vec3 portalFwdVector = portal->GetOrientation().GetForwardDir_IFwd_JLeft_KUp();
	float PtPdotPFwd = DotProduct3D(portalToActor, portalFwdVector);
	if (actor->m_controller != nullptr && actor->m_controller->IsPlayer()) // TODO: Make this work with multiple players
	{
		if (PtPdotPFwd > 0.f)
		{
			portal->m_isPlayerOnFrontSide = true;
		}
		else
		{
			portal->m_isPlayerOnFrontSide = false;
		}
	}
	Mat44 portalTransform = portal->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	Mat44 otherPortalTransform = portal->GetOtherPortal()->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	Vec3 bottomLeft = portalTransform.TransformPosition3D(portal->bl);
	Vec3 bottomRight = portalTransform.TransformPosition3D(portal->br);
	Vec3 topRight = portalTransform.TransformPosition3D(portal->tr);
	Vec3 topLeft = portalTransform.TransformPosition3D(portal->tl);
	RaycastResult3D raycastResult = RaycastVSQuad3D(rayStart, rayFwdNormal, rayLength,
		bottomLeft + portal->GetPosition(),
		bottomRight + portal->GetPosition(),
		topRight + portal->GetPosition(),
		topLeft + portal->GetPosition()
	);
	if (raycastResult.m_didImpact)
	{
		// Calculate the proper position in the other portal space and set the actors desired position to that.
		portalToActor = actor->m_desiredPosition - portal->GetPosition();
		portalTransform.Transpose();
		portalToActor = portalTransform.TransformPosition3D(portalToActor);
		portalToActor = otherPortalTransform.TransformPosition3D(portalToActor);
		actorTranslationThisFrame = portalTransform.TransformPosition3D(actorTranslationThisFrame);
		actorTranslationThisFrame = otherPortalTransform.TransformPosition3D(actorTranslationThisFrame);
		actor->m_desiredPosition = portal->GetOtherPortal()->GetPosition() + portalToActor;

		// Calculate the proper orientation in the other portal space.
		EulerAngles actorOrientation = actor->m_orientation;
		Mat44 selfMatrixWorldToModel = portal->GetWorldToModelTransform();
		Mat44 otherPortalMatrixModelToWorld = portal->GetOtherPortal()->GetModelToWorldTransform();
		selfMatrixWorldToModel.Append(actorOrientation.GetAsMatrix_IFwd_JLeft_KUp()); // Transform the actor's orientation into self model space by appending
		Mat44 newOrientationMatrix = otherPortalMatrixModelToWorld;
		newOrientationMatrix.Append(selfMatrixWorldToModel); // Transform actor's orientation from self model space back to world space with reference to the other portal's space.
		EulerAngles actorOrientationInOtherPortalSpace = EulerAngles(newOrientationMatrix);
		actor->m_orientation = actorOrientationInOtherPortalSpace;
		if (actor->m_controller != nullptr && actor->m_controller->IsPlayer())
		{
			((Player*)actor->m_controller)->m_orientation = actorOrientationInOtherPortalSpace;
		}

		// Calculate proper physics (velocity/acceleration?) in the other portal space.
		Vec3 actorVelocity = actor->m_velocity;
		actorVelocity = portalTransform.TransformPosition3D(actorVelocity);
		actorVelocity = otherPortalTransform.TransformPosition3D(actorVelocity);
		actor->m_velocity = actorVelocity;
		Vec3 actorAcceleration = actor->m_acceleration; // Most likely will be 0.
		actorAcceleration = portalTransform.TransformPosition3D(actorAcceleration);
		actorAcceleration = otherPortalTransform.TransformPosition3D(actorAcceleration);
		actor->m_acceleration = actorAcceleration;

		DebugAddMessage("ENTERED PORTAL", 2.f, Rgba8::RED, Rgba8::RED);
		return true;
	}

	return false;
}

bool Map::PushActorOutOfTileXY(Actor* actor, Tile const& tile)
{
	Vec2 savedPosition = Vec2(actor->m_position.x, actor->m_position.y);

	AABB2 tileBounds = AABB2(
		Vec2(tile.m_bounds.m_mins.x, tile.m_bounds.m_mins.y),
		Vec2(tile.m_bounds.m_maxs.x, tile.m_bounds.m_maxs.y)
	);

	Vec2 actorPosition = Vec2(actor->m_position.x, actor->m_position.y);

	PushDiscOutOfFixedAABB2D(actorPosition, actor->m_definition->m_radius, tileBounds);

	actor->m_position.x = actorPosition.x;
	actor->m_position.y = actorPosition.y;

	if (savedPosition != Vec2(actor->m_position.x, actor->m_position.y))
	{
		return true;
	}
	return false;
}

void Map::DestroyIfGarbage()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* actor = m_actors[actorIndex];
		if (actor != nullptr && actor->m_isGarbage)
		{
			delete actor;
			m_actors[actorIndex] = nullptr;
		}
	}
}

void Map::CollideActorsWithMap()
{
	for (int actorAIndex = 0; actorAIndex < m_actors.size(); ++actorAIndex)
	{
		Actor* actor = m_actors[actorAIndex];
		if (actor != nullptr && 
			!actor->m_isDead && 
			actor->m_definition->m_collidesWithWorld
			)
		{
			CollideActorsWithSurroundingTilesXYZ(actor, actor->m_position - Vec3(0.f, 0.f, 1.f));
			CollideActorsWithSurroundingTilesXYZ(actor, actor->m_position);
			CollideActorsWithSurroundingTilesXYZ(actor, actor->m_position + Vec3(0.f, 0.f, 1.f));
		}
	}
}

void Map::Render()
{
	g_engine->m_render->ClearScreen(m_game->m_backgroundClearColor);

	m_currentlyRenderedPlayer = m_player;
	g_engine->m_render->BeginCamera(m_player->m_worldCamera);

	// Render Everything
	Render_World();

	Render_Portals();

	g_engine->m_render->EndCamera(m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_game->m_screenCamera);

	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	DebugRenderScreen(*m_game->m_screenCamera);

	g_engine->m_render->EndCamera(m_game->m_screenCamera);
}

void Map::Render_World() const
{
	g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);
	Render_Tiles();
	Render_Actors();

	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portal = m_portals[portalIndex];
		if (portal != nullptr)
		{
			//portal->RenderOutline();
		}
	}

	DebugRenderWorld(*m_player->m_worldCamera);
}

void Map::Render_Tiles() const
{
	g_engine->m_render->BindTexture(m_tileSpriteSheet.GetTexture());
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->BindShader(m_definition->m_shader);
	g_engine->m_render->SetLightingConstants(m_sunDirection.GetNormalized(), m_sunIntensity, m_ambientIntensity);
	g_engine->m_render->DrawIndexedVertexList(&m_vertexes, &m_indexes, m_vertexBuffer, m_indexBuffer);
}

void Map::Render_Actors() const
{
	g_engine->m_render->BindTexture(nullptr);
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* actor = m_actors[actorIndex];
		if (actor != nullptr)
		{
			m_actors[actorIndex]->Render();
		}
	}
}

void Map::Render_Portals() const
{
	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portal = m_portals[portalIndex];
		if (portal != nullptr)
		{
			portal->RenderPortal();
		}
	}
}

RaycastResultDoomenstein Map::RaycastAll(const Vec3& start, const Vec3& direction, float distance, [[maybe_unused]] Actor* owner /*= nullptr*/) const
{
	RaycastResult3D raycastResult;
	raycastResult.m_impactDist = distance;

	RaycastResult3D worldXYResult = RaycastWorldXY(start, direction, distance);
	if (worldXYResult.m_impactDist < raycastResult.m_impactDist)
	{
		raycastResult = worldXYResult;
	}
	RaycastResult3D worldZResult = RaycastWorldZ(start, direction, distance);
	if (worldZResult.m_impactDist < raycastResult.m_impactDist)
	{
		raycastResult = worldZResult;
	}

	RaycastResultDoomenstein result;
	result.m_didImpact = raycastResult.m_didImpact;
	result.m_impactDist = raycastResult.m_impactDist;
	result.m_impactNormal = raycastResult.m_impactNormal;
	result.m_impactPos = raycastResult.m_impactPos;
	result.m_rayDirection = raycastResult.m_rayDirection;
	result.m_rayLength = raycastResult.m_rayLength;
	result.m_rayStartPosition = raycastResult.m_rayStartPosition;

	RaycastResultDoomenstein worldActorsResult = RaycastWorldActors(start, direction, distance, owner);
	if (worldActorsResult.m_impactDist < raycastResult.m_impactDist)
	{
		result = worldActorsResult;
	}

	DebugAddWorldCylinder(start, start + direction * distance, Vec3(), 0.01f, 1.f);
	if (raycastResult.m_didImpact)
	{
		DebugAddWorldSphere(raycastResult.m_impactPos, 0.06f, 1.f);
		DebugAddWorldArrow(raycastResult.m_impactPos, raycastResult.m_impactPos + raycastResult.m_impactNormal * 0.3f, Vec3(), 0.03f, 1.f, Rgba8::BLUE, Rgba8::BLUE);
	}

	return result;
}

RaycastResult3D Map::RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const
{
	RaycastResult3D hitResult;
	hitResult.m_impactDist = distance;
	Vec3 startPos = start;
	Vec3 fwdNormal = direction;
	float maxDist = distance;

	IntVec3 coordinates = IntVec3((int)floor(startPos.x), (int)floor(startPos.y), (int)floor(startPos.z));
	int startTileIndex = GetTileIndexFromWorldPosition(startPos);
	//if (startTileIndex == -1)
	//{
	//	return hitResult;
	//}
	if (startTileIndex != -1 && 
		(startPos.z > 0.f && startPos.z < 1.f) &&
		m_tiles[startTileIndex].m_tileDefinition->m_isSolid)
	{
		hitResult.m_didImpact = true;
		hitResult.m_impactPos = startPos;
		hitResult.m_impactNormal = -fwdNormal;
		hitResult.m_impactDist = 0.f;
		return hitResult;
	}

	float fwdDistPerXCrossing = 1.f / abs(fwdNormal.x);
	int tileStepDirectionX = (fwdNormal.x < 0) ? -1 : 1;
	float xAtFirstXCrossing = (float)coordinates.x + (tileStepDirectionX + 1) / 2;
	float xDistToFirstXCrossing = (float)xAtFirstXCrossing - startPos.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing;

	float fwdDistPerYCrossing = 1.f / abs(fwdNormal.y);
	int tileStepDirectionY = (fwdNormal.y < 0) ? -1 : 1;
	float yAtFirstYCrossing = (float)coordinates.y + (tileStepDirectionY + 1) / 2;
	float yDistToFirstYCrossing = (float)yAtFirstYCrossing - startPos.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing;

	float zHeight = (fwdNormal * distance).z;

	while (true)
	{
		// X
		if (fwdDistAtNextXCrossing <= fwdDistAtNextYCrossing)
		{
			float xCrossingTValue = fwdDistAtNextXCrossing / distance;
			float zHeightAtXCrossing = (zHeight * xCrossingTValue) + start.z;
			if (fwdDistAtNextXCrossing >= maxDist)
			{
				RaycastResult3D missResult;
				missResult.m_didImpact = false;
				missResult.m_impactPos = startPos + (fwdNormal * maxDist);
				//missResult.m_impactNormal = ? ? ? ;
				missResult.m_impactDist = maxDist;
				return missResult;
			}
			coordinates.x += tileStepDirectionX;
			IntVec3 intVec3Coordinates = IntVec3(coordinates.x, coordinates.y, RoundDownToInt(zHeightAtXCrossing));
			int tileIndex = GetTileIndexFromCoordinates(intVec3Coordinates);
			if (tileIndex == -1)
			{
				fwdDistAtNextXCrossing += fwdDistPerXCrossing;
				continue;
			}
			Vec3 impactPos = startPos + (fwdNormal * fwdDistAtNextXCrossing);
			if (m_tiles[tileIndex].m_tileDefinition->m_isSolid &&
				impactPos.z > 0.f &&
				impactPos.z < m_dimensions.y
				)
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = fwdDistAtNextXCrossing;
				hitResult.m_impactPos = impactPos;
				if (tileStepDirectionX > 0)
				{
					hitResult.m_impactNormal = Vec3(-1, 0, 0.f);
				}
				else
				{
					hitResult.m_impactNormal = Vec3(1, 0, 0.f);
				}
				return hitResult;
			}
			fwdDistAtNextXCrossing += fwdDistPerXCrossing;
		}
		// Y
		else
		{
			float yCrossingTValue = fwdDistAtNextYCrossing / distance;
			float zHeightAtYCrossing = (zHeight * yCrossingTValue) + start.z;
			if (fwdDistAtNextYCrossing >= maxDist)
			{
				RaycastResult3D missResult;
				missResult.m_didImpact = false;
				missResult.m_impactPos = startPos + (fwdNormal * maxDist);
				//missResult.m_impactNormal = ? ? ? ;
				missResult.m_impactDist = maxDist;
				return missResult;
			}
			coordinates.y += tileStepDirectionY;
			IntVec3 intVec3Coordinates = IntVec3(coordinates.x, coordinates.y, RoundDownToInt(zHeightAtYCrossing));
			int tileIndex = GetTileIndexFromCoordinates(intVec3Coordinates);
			if (tileIndex == -1)
			{
				fwdDistAtNextYCrossing += fwdDistPerYCrossing;
				continue;
			}
			Vec3 impactPos = startPos + (fwdNormal * fwdDistAtNextYCrossing);
			if (m_tiles[tileIndex].m_tileDefinition->m_isSolid &&
				impactPos.z > 0.f &&
				impactPos.z < m_dimensions.y
				)
			{
				hitResult.m_didImpact = true;
				hitResult.m_impactDist = fwdDistAtNextYCrossing;
				hitResult.m_impactPos = impactPos;
				if (tileStepDirectionY > 0)
				{
					hitResult.m_impactNormal = Vec3(0, -1, 0.f);
				}
				else
				{
					hitResult.m_impactNormal = Vec3(0, 1, 0.f);
				}
				return hitResult;
			}
			fwdDistAtNextYCrossing += fwdDistPerYCrossing;
		}
	}
}

RaycastResult3D Map::RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const
{
	RaycastResult3D result;
	result.m_impactDist = distance;

	Vec3 rayEnd = start + direction * distance;

	for (int zLevelIndex = 0; zLevelIndex < m_definition->m_mapImages.size(); ++zLevelIndex)
	{
		float rayZLength = rayEnd.z - start.z;
		float zTFloor = ((float)zLevelIndex + 0.f - start.z) / rayZLength;
		float zTCeiling = ((float)zLevelIndex + 1.f - start.z) / rayZLength;
		Vec3 floorHitPoint = start + direction * distance * zTFloor;
		Vec3 ceilingHitPoint = start + direction * distance * zTCeiling;

		AABB2 mapBounds = AABB2(Vec2(0.f, 0.f), Vec2((float)m_dimensions.x, (float)m_dimensions.y));

		FloatRange zRange = FloatRange(start.z, rayEnd.z);
		if (start.z > rayEnd.z)
		{
			zRange.m_min = rayEnd.z;
			zRange.m_max = start.z;
		}
		float floorImpactDist = (floorHitPoint - start).GetLength();
		float ceilingImpactDist = (ceilingHitPoint - start).GetLength();

		int tileIndex = GetTileIndexFromCoordinates(IntVec3(RoundDownToInt(floorHitPoint.x), RoundDownToInt(floorHitPoint.y), zLevelIndex));
		if (tileIndex == -1)
		{
			continue;
		}
		Tile currentTile = m_tiles[tileIndex];

		// Did hit floor
		if (zRange.IsOnRange((float)zLevelIndex + 0.f) &&
			start.z > (float)zLevelIndex + 0.f &&
			mapBounds.IsPointInside(Vec2(floorHitPoint.x, floorHitPoint.y)) &&
			floorImpactDist < result.m_impactDist &&
			currentTile.m_tileDefinition->m_floorSpriteCoords != IntVec2(-1, -1)
			)
		{
			result.m_didImpact = true;
			result.m_impactPos = floorHitPoint;
			result.m_impactDist = floorImpactDist;
			result.m_impactNormal = Vec3(0.f, 0.f, 1.f);
		}
		// Did hit ceiling
		if (zRange.IsOnRange((float)zLevelIndex + 1.f) &&
			start.z < (float)zLevelIndex + 1.f &&
			mapBounds.IsPointInside(Vec2(ceilingHitPoint.x, ceilingHitPoint.y)) &&
			ceilingImpactDist < result.m_impactDist &&
			currentTile.m_tileDefinition->m_ceilingSpriteCoords != IntVec2(-1, -1)
			)
		{
			result.m_didImpact = true;
			result.m_impactPos = ceilingHitPoint;
			result.m_impactDist = ceilingImpactDist;
			result.m_impactNormal = Vec3(0.f, 0.f, -1.f);
		}
	}

	return result;
}

RaycastResultDoomenstein Map::RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, [[maybe_unused]] Actor* owner /*= nullptr*/) const
{
	RaycastResult3D raycastResult;
	raycastResult.m_impactDist = distance;
	Actor* actorHit = nullptr;

	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* actor = m_actors[actorIndex];
		if (actor != nullptr && !actor->m_isDead && actor != owner)
		{
			RaycastResult3D newRaycastResult = RaycastVsCylinderZ3D(start, direction, distance, Vec2(actor->m_position.x, actor->m_position.y), actor->m_position.z, actor->m_position.z + actor->m_definition->m_height, actor->m_definition->m_radius);
			if (newRaycastResult.m_impactDist != 0.f && newRaycastResult.m_impactDist < raycastResult.m_impactDist)
			{
				raycastResult = newRaycastResult;
				actorHit = actor;
			}
		}
	}

	RaycastResultDoomenstein result;
	result.m_actor = actorHit;
	result.m_didImpact = raycastResult.m_didImpact;
	result.m_impactDist = raycastResult.m_impactDist;
	result.m_impactNormal = raycastResult.m_impactNormal;
	result.m_impactPos = raycastResult.m_impactPos;
	result.m_rayDirection = raycastResult.m_rayDirection;
	result.m_rayLength = raycastResult.m_rayLength;
	result.m_rayStartPosition = raycastResult.m_rayStartPosition;
	return result;
}

void Map::Debug_KillAllActors()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* actor = m_actors[actorIndex];
		if (actor != nullptr)
		{
			m_actors[actorIndex]->Die();
		}
	}
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

		XmlElement* mapImages = mapDefElement->FirstChildElement("MapImages");
		XmlElement* MapImageElement = mapImages->FirstChildElement();
		while (MapImageElement)
		{
			std::string mapImagePath = ParseXmlAttribute(*MapImageElement, "image", "");
			newMapDef->m_mapImages.push_back(new Image(mapImagePath.data()));
			MapImageElement = MapImageElement->NextSiblingElement();
		}

		XmlElement* mapSpawnInfos = mapDefElement->FirstChildElement("SpawnInfos");
		XmlElement* mapSpawnInfoElement = mapSpawnInfos->FirstChildElement();
		while (mapSpawnInfoElement)
		{
			SpawnInfo newSpawnInfo = SpawnInfo(ParseXmlAttribute(*mapSpawnInfoElement, "actor", ""), ParseXmlAttribute(*mapSpawnInfoElement, "position", Vec3()), ParseXmlAttribute(*mapSpawnInfoElement, "orientation", EulerAngles()));
			newMapDef->m_spawnInfos.push_back(newSpawnInfo);
			mapSpawnInfoElement = mapSpawnInfoElement->NextSiblingElement();
		}

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
	return nullptr;
}
