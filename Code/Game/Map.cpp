#include "Game/Map.hpp"

#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/Actor.hpp"

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

std::vector<MapDefinition*> MapDefinition::s_definitions;

Map::Map(Game* game, const MapDefinition* definition)
	:m_game(game)
	, m_definition(definition)
{
	Startup();
}

Map::~Map()
{
	delete m_player;
	delete m_playerTranslationThisFrame;
	delete m_vertexBuffer;
	delete m_indexBuffer;

	m_player = nullptr;
	m_playerTranslationThisFrame = nullptr;
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
}

void Map::Startup()
{
	m_tileSpriteSheet = SpriteSheet(m_definition->m_spriteSheetTexture, m_definition->m_spriteSheetCellCount);
	m_dimensions = m_definition->m_mapImage->GetDimensions();
	CreateTiles();
	CreateGeometry();
	CreateBuffers();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Initialize Player
	m_player = new Player(this, Vec3(-5.f, 0.f, 1.f));
	m_player->m_worldCamera = new Camera();
	m_player->m_screenCamera = new Camera();

	m_player->m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.1f, 100.f);
	m_player->m_worldCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);
	m_player->m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_player->m_position = Vec3(2.5f, 8.5f, 0.5f);

	m_playerTranslationThisFrame = new Vec3();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Initialize Actors
	Actor* actor = new Actor(
		Vec3(7.5f, 8.5f, 0.25f),
		0.75f,
		0.35f,
		EulerAngles(),
		Rgba8::RED
	);
	AddActorToMap(actor);
	actor->setStatic(true);
	actor = new Actor(
		Vec3(8.5f, 8.5f, 0.125f),
		0.75f,
		0.35f,
		EulerAngles(),
		Rgba8::RED);
	AddActorToMap(actor);
	actor->setStatic(true);
	actor = new Actor(
		Vec3(9.5f, 8.5f, 0.f),
		0.75f,
		0.35f,
		EulerAngles(),
		Rgba8::RED);
	AddActorToMap(actor);
	actor->setStatic(true);

	// Test Projectile
	actor = new Actor(
		Vec3(5.5f, 8.5f, 0.f),
		0.125f,
		0.0625f,
		EulerAngles(),
		Rgba8::BLUE);
	AddActorToMap(actor);
	actor->setStatic(false);
	m_bulletActor = actor;

	m_sunDirection = Vec3(2.f, 1.f, -1.f);
	m_sunIntensity = 0.85f;
	m_ambientIntensity = 0.35f;
}

void Map::CreateTiles()
{
	for (int xCoord = 0; xCoord < m_dimensions.x; ++xCoord)
	{
		for (int yCoord = 0; yCoord < m_dimensions.y; ++yCoord)
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
	m_vertexBuffer = g_engine->m_render->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_engine->m_render->CreateIndexBuffer(sizeof(unsigned int));
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

int Map::GetTileIndexFromWorldPosition(Vec3 position)
{
	int indexToReturn = (RoundDownToInt(position.x) * m_dimensions.y) + RoundDownToInt(position.y);
	if (indexToReturn < 0 || indexToReturn > m_tiles.size() - 1)
	{
		return -1;
	}
	return indexToReturn;
}

void Map::AddActorToMap(Actor* actor)
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		Actor* currentActor = m_actors[actorIndex];
		if (currentActor == nullptr)
		{
			currentActor = actor;
			return;
		}
	}
	m_actors.push_back(actor);
}

void Map::Update()
{
	bool didGameReset = false;
	if (m_isControllingBullet)
	{
		didGameReset = Update_KeyboardInputBullet();
	}
	else
	{
		didGameReset = Update_KeyboardInput();
	}
	if (didGameReset)
	{
		return;
	}

	didGameReset = Update_ControllerInput();
	if (didGameReset)
	{
		return;
	}

	Update_DebugInput();
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Update Actors
	Update_Actors();
	CollideActors();
	CollideActorsWithMap();

	m_player->Update();
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

	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		RaycastAll(m_player->m_position, m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), 10.f, nullptr);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		RaycastAll(m_player->m_position, m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), 0.25f, nullptr);
	}
}

void Map::Update_Actors()
{
	for (int actorIndex = 0; actorIndex < m_actors.size(); ++actorIndex)
	{
		m_actors[actorIndex]->Update();
	}
}

void Map::CollideActors()
{
	for (int actorAIndex = 0; actorAIndex < m_actors.size(); ++actorAIndex)
	{
		Actor* actorA = m_actors[actorAIndex];
		for (int actorBIndex = actorAIndex + 1; actorBIndex < m_actors.size(); ++actorBIndex)
		{
			Actor* actorB = m_actors[actorBIndex];
			CollideActors(actorA, actorB);
		}
	}
}

void Map::CollideActors(Actor* actorA, Actor* actorB)
{
	FloatRange actorAZRange = FloatRange(actorA->m_position.z, actorA->m_position.z + actorA->m_physicsHeight);
	FloatRange actorBZRange = FloatRange(actorB->m_position.z, actorB->m_position.z + actorB->m_physicsHeight);
	if (!actorAZRange.IsOverlappingWith(actorBZRange))
	{
		return;
	}

	Vec2 actorADisc = Vec2(actorA->m_position.x, actorA->m_position.y);
	Vec2 actorBDisc = Vec2(actorB->m_position.x, actorB->m_position.y);
	PushDiscsOutOfEachOther2D(actorADisc, actorA->m_physicsRadius, actorBDisc, actorB->m_physicsRadius);
	actorA->m_position.x = actorADisc.x;
	actorA->m_position.y = actorADisc.y;
	actorB->m_position.x = actorBDisc.x;
	actorB->m_position.y = actorBDisc.y;
}

void Map::CollideActorsWithSurroundingTilesXY(Actor* actor)
{
	Vec3 NORTH		= Vec3(0.f, 1.f, 0.f);
	Vec3 NORTH_EAST	= Vec3(1.f, 1.f, 0.f);
	Vec3 EAST		= Vec3(1.f, 0.f, 0.f);
	Vec3 SOUTH_EAST = Vec3(1.f, -1.f, 0.f);
	Vec3 SOUTH		= Vec3(0.f, -1.f, 0.f);
	Vec3 SOUTH_WEST = Vec3(-1.f, -1.f, 0.f);
	Vec3 WEST		= Vec3(-1.f, 0.f, 0.f);
	Vec3 NORTH_WEST = Vec3(-1.f, 1.f, 0.f);

	CollideActorWithSingleTileXY(actor, actor->m_position);
	CollideActorWithSingleTileXY(actor, actor->m_position + NORTH);
	CollideActorWithSingleTileXY(actor, actor->m_position + NORTH_EAST);
	CollideActorWithSingleTileXY(actor, actor->m_position + EAST);
	CollideActorWithSingleTileXY(actor, actor->m_position + SOUTH_EAST);
	CollideActorWithSingleTileXY(actor, actor->m_position + SOUTH);
	CollideActorWithSingleTileXY(actor, actor->m_position + SOUTH_WEST);
	CollideActorWithSingleTileXY(actor, actor->m_position + WEST);
	CollideActorWithSingleTileXY(actor, actor->m_position + NORTH_WEST);
}

void Map::CollideActorWithSingleTileXY(Actor* actor, Vec3 tilePosition)
{
	int tileIndex = GetTileIndexFromWorldPosition(tilePosition);
	if (tileIndex == -1)
	{
		return;
	}
	Tile tile = m_tiles[tileIndex];
	if (tile.m_tileDefinition->m_isSolid)
	{
		PushActorOutOfTileXY(actor, tile);
	}
}

void Map::PushActorOutOfTileXY(Actor* actor, Tile const& tile)
{
	AABB2 tileBounds = AABB2(
		Vec2(tile.m_bounds.m_mins.x, tile.m_bounds.m_mins.y),
		Vec2(tile.m_bounds.m_maxs.x, tile.m_bounds.m_maxs.y)
	);

	Vec2 actorPosition = Vec2(actor->m_position.x, actor->m_position.y);

	PushDiscOutOfFixedAABB2D(actorPosition, actor->m_physicsRadius, tileBounds);

	actor->m_position.x = actorPosition.x;
	actor->m_position.y = actorPosition.y;
}

void Map::CollideActorsWithSurroundingCeilingsAndFloorsXY(Actor* actor)
{
	FloatRange actorZRange = FloatRange(actor->m_position.z, actor->m_position.z + actor->m_physicsHeight);
	// IsCollidingWithCeiling
	if (actorZRange.IsOnRange(1.f))
	{
		actor->m_position.z = 1.f - actor->m_physicsHeight;
	}
	// IsCollidingWithFloor
	if (actorZRange.IsOnRange(0.f))
	{
		actor->m_position.z = 0.f;
	}
}

void Map::CollideActorsWithMap()
{
	for (int actorAIndex = 0; actorAIndex < m_actors.size(); ++actorAIndex)
	{
		Actor* actor = m_actors[actorAIndex];
		CollideActorsWithSurroundingTilesXY(actor);
		CollideActorsWithSurroundingCeilingsAndFloorsXY(actor);
	}
}

void Map::CollideActorsWithMap(Actor* actor)
{

}

void Map::Render() const
{
	g_engine->m_render->ClearScreen(m_game->m_backgroundClearColor);

	g_engine->m_render->BeginCamera(m_player->m_worldCamera);

	// Render Everything
	Render_Tiles();
	Render_Actors();
	DebugRenderWorld(*m_player->m_worldCamera);

	g_engine->m_render->EndCamera(m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_player->m_screenCamera);

	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	DebugRenderScreen(*m_player->m_screenCamera);

	g_engine->m_render->EndCamera(m_player->m_screenCamera);
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
		m_actors[actorIndex]->Render();
	}
}

RaycastResult3D Map::RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor* owner /*= nullptr*/) const
{
	RaycastResult3D result;

	RaycastResult3D worldXYResult = RaycastWorldXY(start, direction, distance);
	if (worldXYResult.m_impactDist < result.m_impactDist)
	{
		result = worldXYResult;
	}
	RaycastResult3D worldZResult = RaycastWorldZ(start, direction, distance);
	if (worldZResult.m_impactDist < result.m_impactDist)
	{
		result = worldZResult;
	}
	RaycastResult3D worldActorsResult = RaycastWorldActors(start, direction, distance);
	if (worldActorsResult.m_impactDist < result.m_impactDist)
	{
		result = worldActorsResult;
	}

	DebugAddWorldCylinder(start, start + direction * distance, Vec3(), 0.01f, 10.f);
	if (result.m_didImpact)
	{
		DebugAddWorldSphere(result.m_impactPos, 0.06f, 10.f);
		DebugAddWorldArrow(result.m_impactPos, result.m_impactPos + result.m_impactNormal * 0.3f, Vec3(), 0.03f, 10.f, Rgba8::BLUE);
	}

	return result;
}

RaycastResult3D Map::RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const
{
	RaycastResult3D result;



	return result;
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
