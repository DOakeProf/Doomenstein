#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Player.hpp"
#include "Game/Map.hpp"
#include "Game/Tile.hpp"
#include "Game/Weapon.hpp"
#include "Game/Actor.hpp"

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
#include "Engine/BitmapFont.hpp"
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
		EnterState(m_nextGameState);
		ExitState(m_currentGameState);
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
	WeaponDefinition::InitializeDefinitions("Data/Definitions/WeaponDefinitions.xml");
	ActorDefinition::InitializeDefinitions("Data/Definitions/ActorDefinitions.xml");
	ActorDefinition::InitializeDefinitions("Data/Definitions/ProjectileActorDefinitions.xml");
	Startup_PopulateFromBlackboard();

	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);

	m_screenCamera = new Camera();
	m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Initialize Player
	m_player = new Player(nullptr, Vec3(2.5f, 8.5f, 0.5f));
	m_player->m_worldCamera = new Camera();

	m_player->m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.1f, 100.f);
	m_player->m_worldCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);

	m_squirrelFont = g_engine->m_render->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
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
	m_perspectiveFOV = g_gameConfigBlackboard.GetValue("perspectiveFOV", 0.f);
	m_rollSensitivity = g_gameConfigBlackboard.GetValue("rollSensitivity", 0.f);
	m_mouseSensitivity = g_gameConfigBlackboard.GetValue("mouseSensitivity", 0.f);
	m_controllerSensitivity = g_gameConfigBlackboard.GetValue("controllerSensitivity", 0.f);
	m_moveSpeed = g_gameConfigBlackboard.GetValue("moveSpeed", 0.f);

	m_mapDefinitionString = g_gameConfigBlackboard.GetValue("defaultMap", "");
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

	g_engine->m_render->BeginCamera(m_screenCamera);

	std::vector<Vertex> verts;

	m_squirrelFont->AddVertsForTextInBox2D(verts, "Press SPACE to start", AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), SCREEN_SIZE_Y * 0.03f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.1f));

	g_engine->m_render->BindTexture(&m_squirrelFont->GetTexture());
	g_engine->m_render->DrawVertexList(&verts);

	g_engine->m_render->EndCamera(m_screenCamera);
}

void Game::Render_LobbyMode() const
{

}

void Game::Update_PlayingMode()
{
	// Entity updates
	m_currentMap->Update();
	m_player->Update();

	// Camera updates
}

void Game::Render_PlayingMode() const
{
	m_currentMap->Render();
}

void Game::ChangeGameState(GameState newGameState)
{
	m_nextGameState = newGameState;
}

void Game::EnterState(GameState state)
{
	switch (state)
	{
		case GameState::GAME_STATE_PLAYING:
		{
			m_currentMap = new Map(this, MapDefinition::GetByName(m_mapDefinitionString), m_player);
			m_currentMap->Startup_InitializeActors(); // Initialize actors here because the function calls things that only exist after the map is made.
			break;
		}
		case GameState::GAME_STATE_ATTRACT:
		{
			g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
			g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			break;
		}
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
		case GameState::GAME_STATE_PLAYING:
		{
			delete m_currentMap;
			m_currentMap = nullptr;
			break;
		}
	}
}