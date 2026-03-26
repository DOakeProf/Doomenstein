#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Player.hpp"
#include "Game/Map.hpp"
#include "Game/Tile.hpp"

#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/Texture.hpp"
// TODO: Make gamestate set by an enum value. DO current state and next state, 
// when these are different know we're switching states, run current state frame 
// and then next frame switch. switch at top of update. enum has invalidstate as -1. 
// Default is invalid state.

Game::Game()
{
	Startup();
}

Game::~Game()
{
	delete m_randomNumberGenerator;

	m_randomNumberGenerator = nullptr;
}

void Game::Update()
{
	if (m_nextGameState != m_currentGameState)
	{
		m_currentGameState = m_nextGameState;
	}

	switch (m_currentGameState)
	{
		case GameState::GAME_STATE_ATTRACT:		Update_AttractMode(); break;
		case GameState::GAME_STATE_LOBBY:		Update_LobbyMode(); break;
		case GameState::GAME_STATE_PLAYING:		Update_PlayingMode(); break;
	}
}

void Game::Render() const
{
	switch (m_currentGameState)
	{
		case GameState::GAME_STATE_ATTRACT:		Render_AttractMode(); break;
		case GameState::GAME_STATE_LOBBY:		Render_LobbyMode(); break;
		case GameState::GAME_STATE_PLAYING:		Render_PlayingMode(); break;
	}
}

void Game::Startup()
{
	m_randomNumberGenerator = new RandomNumberGenerator();
	m_gameClock = new Clock();

	MapDefinition::InitializeDefinitions("Data/Definitions/MapDefinitions.xml");
	TileDefinition::InitializeDefinitions("Data/Definitions/TileDefinitions.xml");
	Startup_PopulateFromBlackboard();

	m_player = new Player(this, Vec3(-5.f, 0.f, 1.f));
	m_playerTranslationThisFrame = new Vec3();

	m_player->m_worldCamera = new Camera(Vec2(-1, -1), Vec2(1, 1));
	m_player->m_screenCamera = new Camera();

	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);

	m_player->m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.1f, 100.f);
	m_player->m_worldCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);

	m_player->m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game::AddScreenShake(float screenShake)
{
	m_screenShakeFraction += screenShake;
}

void Game::AddHitStop(float hitStop)
{
	m_hitStopTimer += hitStop;
	m_isHitStop = true;
}

void Game::Startup_PopulateFromBlackboard()
{
	//m_perspectiveFOV = g_gameConfigBlackboard.GetValue("perspectiveFOV", 0.f);
	//m_rollSensitivity = g_gameConfigBlackboard.GetValue("rollSensitivity", 0.f);
	//m_mouseSensitivity = g_gameConfigBlackboard.GetValue("mouseSensitivity", 0.f);
	//m_controllerSensitivity = g_gameConfigBlackboard.GetValue("controllerSensitivity", 0.f);
	//m_moveSpeed = g_gameConfigBlackboard.GetValue("moveSpeed", 0.f);
	//m_colorUndulateTime = g_gameConfigBlackboard.GetValue("colorUndulateTime", 0.f);

	std::string mapDefinitionString = g_gameConfigBlackboard.GetValue("defaultMap", "");
	m_currentMap = new Map(this, MapDefinition::GetByName(mapDefinitionString));
}

void Game::Update_AttractMode()
{
	// Keyboard Inputs
	if (g_engine->m_input->WasKeyJustPressed(' ') or g_engine->m_input->WasKeyJustPressed('N'))
	{
		ChangeGameState(GameState::GAME_STATE_PLAYING);
	}

	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_app->SetIsQuitting();
	}

	// Xbox Controller Inputs
	if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::GAMEPAD_B) or g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::GAMEPAD_A))
	{
		ChangeGameState(GameState::GAME_STATE_PLAYING);
	}

	if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::BACK))
	{
		g_app->SetIsQuitting();
	}
}

void Game::Update_LobbyMode()
{

}

void Game::Render_AttractMode() const
{
	g_engine->m_render->ClearScreen(Rgba8(0, 0, 0, 255));

	g_engine->m_render->BeginCamera(m_player->m_screenCamera);

	Vertex vertices[] = {
		Vertex(Vec3(600.f, 300.f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
		Vertex(Vec3(1000.f, 300.f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
		Vertex(Vec3(800.f, 600.f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f))
	};

	g_engine->m_render->BindTexture(NULL);
	g_engine->m_render->DrawVertexArray(3, vertices);

	g_engine->m_render->EndCamera(m_player->m_screenCamera);
}

void Game::Render_LobbyMode() const
{

}

void Game::Update_PlayingMode()
{
	//float deltaSeconds = (float)m_gameClock->GetDeltaSeconds();

	bool didGameReset = Update_MainMode_KeyboardInput();
	if (didGameReset)
	{
		return;
	}

	didGameReset = Update_MainMode_ControllerInput();
	if (didGameReset)
	{
		return;
	}

	// Entity updates
	m_player->Update();

	// Camera updates
	if (m_player->m_worldCamera != nullptr)
	{
		m_player->m_worldCamera->SetPerspectiveView(g_engine->m_window->m_config.m_clientAspect, m_perspectiveFOV, 0.1f, 100.f);
	}
	if (m_player->m_screenCamera != nullptr)
	{
		m_player->m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	}
}

void Game::Render_PlayingMode() const
{
	g_engine->m_render->ClearScreen(m_backgroundClearColor);

	g_engine->m_render->BeginCamera(m_player->m_worldCamera);

	// Render Everything

	g_engine->m_render->EndCamera(m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_player->m_screenCamera);

	DebugRenderScreen(*m_player->m_screenCamera);

	g_engine->m_render->EndCamera(m_player->m_screenCamera);
}

bool Game::Update_MainMode_KeyboardInput()
{
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_app->GameReset();
		return true;
	}

	if (g_engine->m_input->IsKeyDown('T'))
	{
		m_gameClock->SetTimeScale(0.1f);
	}
	else
	{
		m_gameClock->SetTimeScale(1.f);
	}

	float currentMoveSpeed = m_moveSpeed;
	if (g_engine->m_input->IsKeyDown(KEYCODE_SHIFT))
	{
		currentMoveSpeed *= 10.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_player->m_orientation;
	newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_mouseSensitivity;
	newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_mouseSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	if (g_engine->m_input->IsKeyDown('Q'))
	{
		newOrientation.m_rollDegrees -= (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;
	}
	if (g_engine->m_input->IsKeyDown('E'))
	{
		newOrientation.m_rollDegrees += (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;
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

	if (g_engine->m_input->IsKeyDown('H'))
	{
		m_player->m_position = Vec3();
		m_player->m_orientation = EulerAngles();
	}

	if (g_engine->m_input->WasKeyJustPressed('P'))
	{
		m_gameClock->TogglePause();
	}

	if (g_engine->m_input->WasKeyJustPressed('O'))
	{
		m_gameClock->StepSingleFrame();
	}

	return false;
}

bool Game::Update_MainMode_ControllerInput()
{
	XboxController* controller = &g_engine->m_input->m_controllers[0];
	if (controller->WasButtonJustPressed(XboxButtonID::BACK))
	{
		g_app->GameReset();
		return true;
	}

	float currentMoveSpeed = m_moveSpeed;
	if (controller->IsButtonDown(XboxButtonID::GAMEPAD_A))
	{
		currentMoveSpeed *= 10.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_player->m_orientation;
	newOrientation.m_yawDegrees -= controller->GetRightStick().GetPosition().x * m_controllerSensitivity;
	newOrientation.m_pitchDegrees -= controller->GetRightStick().GetPosition().y * m_controllerSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	
	newOrientation.m_rollDegrees += controller->GetRightTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;
	newOrientation.m_rollDegrees -= controller->GetLeftTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;

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

void Game::ChangeGameState(GameState newGameState)
{
	m_nextGameState = newGameState;
}

