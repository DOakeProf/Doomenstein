#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Player.hpp"
#include "Game/Map.hpp"
#include "Game/Tile.hpp"
#include "Game/Weapon.hpp"
#include "Game/Actor.hpp"
#include "Game/Rift.hpp"

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

static std::vector<Rift*> s_rifts;

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
	g_engine->m_render->ClearScreen(m_backgroundClearColor);
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
	m_screenCamera->SetViewport(AABB2(0.f, 0.f, (float)g_engine->m_window->GetClientDimensions().x, (float)g_engine->m_window->GetClientDimensions().y));

	m_squirrelFont = g_engine->m_render->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	m_useTexture1Shader = g_engine->m_render->CreateShader("Data/Shaders/PortalShader", VertexType::VERTEX_PCU);
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

	m_musicVolume = g_gameConfigBlackboard.GetValue("musicVolume", 0.1f);
	m_mainMenuMusic = g_engine->m_audio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("mainMenuMusic", ""), true);
	m_gameMusic = g_engine->m_audio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("gameMusic", ""), true);
	m_buttonClickSound = g_engine->m_audio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("buttonClickSound", ""), true);

	m_mapDefinitionString = g_gameConfigBlackboard.GetValue("defaultMap", "");
	m_riftMapDefinitionString = g_gameConfigBlackboard.GetValue("defaultRiftMap", "");
}

void Game::Update_AttractMode()
{
	// Keyboard Inputs
	if (g_engine->m_input->WasKeyJustPressed(' '))
	{
		ChangeGameState(GameState::GAME_STATE_LOBBY);
		JoinPlayer(-1);
		g_engine->m_audio->StartSound(m_buttonClickSound);
	}

	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_app->SetIsQuitting();
	}

	// Xbox Controller Inputs
	if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::START))
	{
		ChangeGameState(GameState::GAME_STATE_LOBBY);
		JoinPlayer(0);
		g_engine->m_audio->StartSound(m_buttonClickSound);
	}

	if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::BACK))
	{
		g_app->SetIsQuitting();
	}
}

void Game::Update_LobbyMode()
{
	int playerSize = 0;
	for (Player* player : m_players)
	{
		if (player != nullptr)
		{
			++playerSize;
		}
	}

	if (playerSize == 0)
	{
		return;
	}
	else if (playerSize == 1)
	{
		int onlyPlayer = 0;
		if (m_players[onlyPlayer] == nullptr)
		{
			onlyPlayer = 1;
		}

		// Keyboard Inputs
		if (g_engine->m_input->WasKeyJustPressed(' '))
		{
			if (m_players[onlyPlayer]->m_controllerIndex == -1)
			{
				ChangeGameState(GameState::GAME_STATE_PLAYING);
				g_engine->m_audio->StartSound(m_buttonClickSound);
			}
			else
			{
				JoinPlayer(-1);
				g_engine->m_audio->StartSound(m_buttonClickSound);
			}
		}

		if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
		{
			if (m_players[onlyPlayer]->m_controllerIndex == -1)
			{
				delete m_players[onlyPlayer];
				m_players[onlyPlayer] = nullptr;
			}
			ChangeGameState(GameState::GAME_STATE_ATTRACT);
			g_engine->m_audio->StartSound(m_buttonClickSound);
		}

		// Xbox Controller Inputs
		if (g_engine->m_input->m_controllers[onlyPlayer].WasButtonJustPressed(XboxButtonID::START))
		{
			if (m_players[onlyPlayer]->m_controllerIndex == 0)
			{
				ChangeGameState(GameState::GAME_STATE_PLAYING);
				g_engine->m_audio->StartSound(m_buttonClickSound);
			}
			else
			{
				JoinPlayer(0);
				g_engine->m_audio->StartSound(m_buttonClickSound);
			}
		}

		if (g_engine->m_input->m_controllers[onlyPlayer].WasButtonJustPressed(XboxButtonID::BACK))
		{
			if (m_players[onlyPlayer]->m_controllerIndex == 0)
			{
				delete m_players[onlyPlayer];
				m_players[onlyPlayer] = nullptr;
			}
			ChangeGameState(GameState::GAME_STATE_ATTRACT);
			g_engine->m_audio->StartSound(m_buttonClickSound);
		}

	}
	else
	{
		// Keyboard Inputs
		if (g_engine->m_input->WasKeyJustPressed(' '))
		{
			ChangeGameState(GameState::GAME_STATE_PLAYING);
			g_engine->m_audio->StartSound(m_buttonClickSound);
		}

		if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
		{
			for (int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex)
			{
				if (m_players[playerIndex]->m_controllerIndex == -1)
				{
					delete m_players[playerIndex];
					m_players[playerIndex] = nullptr;
				}
			}
			g_engine->m_audio->StartSound(m_buttonClickSound);
		}

		// Xbox Controller Inputs
		if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::START))
		{
			ChangeGameState(GameState::GAME_STATE_PLAYING);
			g_engine->m_audio->StartSound(m_buttonClickSound);
		}

		if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::BACK))
		{
			for (int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex)
			{
				if (m_players[playerIndex]->m_controllerIndex == 0)
				{
					delete m_players[playerIndex];
					m_players[playerIndex] = nullptr;
				}
			}
			g_engine->m_audio->StartSound(m_buttonClickSound);
		}
	}
}

void Game::Render_AttractMode() const
{
	g_engine->m_render->ClearScreen(Rgba8(0, 0, 0, 255));

	g_engine->m_render->BeginCamera(m_screenCamera);

	std::vector<Vertex> verts;

	m_squirrelFont->AddVertsForTextInBox2D(verts, "Press SPACE to join with mouse and keyboard\nPress START to join with controller\nPress ESCAPE or BACK to exit", AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), SCREEN_SIZE_Y * 0.03f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.1f));

	g_engine->m_render->BindTexture(&m_squirrelFont->GetTexture());
	g_engine->m_render->DrawVertexList(&verts);

	g_engine->m_render->EndCamera(m_screenCamera);
}

void Game::Render_LobbyMode() const
{
	g_engine->m_render->ClearScreen(Rgba8(0, 0, 0, 255));

	g_engine->m_render->BeginCamera(m_screenCamera);

	std::vector<Vertex> verts;

	AABB2 screenAABB = AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	int playerSize = 0;
	for (Player* player : m_players)
	{
		if (player != nullptr)
		{
			++playerSize;
		}
	}

	if (playerSize == 0)
	{
		return;
	}
	else if (playerSize < 2)
	{
		int onlyPlayer = 0;
		if (m_players[0] == nullptr)
		{
			onlyPlayer = 1;
		}

		std::string player1Control;
		std::string player1Commands;

		if (m_players[onlyPlayer]->m_controllerIndex == -1)
		{
			player1Control = "Mouse and Keyboard";
		}
		else
		{
			player1Control = "Controller";
		}

		if (m_players[onlyPlayer]->m_controllerIndex == -1)
		{
			player1Commands = "Press SPACE to start game\nPress ESCAPE to leave game\nPress START to join player";
		}
		else
		{
			player1Commands = "Press START to start game\nPress BACK to leave game\nPress SPACE to join player";
		}

		m_squirrelFont->AddVertsForTextInBox2D(verts, "Player 1", screenAABB, SCREEN_SIZE_Y * 0.05f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.5f));
		m_squirrelFont->AddVertsForTextInBox2D(verts, player1Control, screenAABB, SCREEN_SIZE_Y * 0.03f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.45f));
		m_squirrelFont->AddVertsForTextInBox2D(verts, player1Commands, screenAABB, SCREEN_SIZE_Y * 0.02f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.35f));
	}
	else
	{
		AABB2 player1AABB = screenAABB;
		player1AABB.DivideHorizontal(0.5f, false);
		AABB2 player2AABB = screenAABB;
		player2AABB.DivideHorizontal(0.5f, true);

		std::string player1Control;
		std::string player1Commands;

		if (m_players[0]->m_controllerIndex == -1)
		{
			player1Control = "Mouse and Keyboard";
		}
		else
		{
			player1Control = "Controller";
		}

		if (m_players[0]->m_controllerIndex == -1)
		{
			player1Commands = "Press SPACE to start game\nPress ESCAPE to leave game";
		}
		else
		{
			player1Commands = "Press START to start game\nPress BACK to leave game";
		}

		std::string player2Control;
		std::string player2Commands;

		if (m_players[1]->m_controllerIndex == -1)
		{
			player2Control = "Mouse and Keyboard";
		}
		else
		{
			player2Control = "Controller";
		}

		if (m_players[1]->m_controllerIndex == -1)
		{
			player2Commands = "Press SPACE to start game\nPress ESCAPE to leave game";
		}
		else
		{
			player2Commands = "Press START to start game\nPress BACK to leave game";
		}

		m_squirrelFont->AddVertsForTextInBox2D(verts, "Player 1", player1AABB, SCREEN_SIZE_Y * 0.05f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.5f));
		m_squirrelFont->AddVertsForTextInBox2D(verts, player1Control, player1AABB, SCREEN_SIZE_Y * 0.03f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
		m_squirrelFont->AddVertsForTextInBox2D(verts, player1Commands, player1AABB, SCREEN_SIZE_Y * 0.02f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.25f));

		m_squirrelFont->AddVertsForTextInBox2D(verts, "Player 2", player2AABB, SCREEN_SIZE_Y * 0.05f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.5f));
		m_squirrelFont->AddVertsForTextInBox2D(verts, player2Control, player2AABB, SCREEN_SIZE_Y * 0.03f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
		m_squirrelFont->AddVertsForTextInBox2D(verts, player2Commands, player2AABB, SCREEN_SIZE_Y * 0.02f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.25f));
	}

	g_engine->m_render->BindTexture(&m_squirrelFont->GetTexture());
	g_engine->m_render->DrawVertexList(&verts);

	g_engine->m_render->EndCamera(m_screenCamera);
}

void Game::Update_PlayingMode()
{
	// Entity updates
	for (Player* player : m_players)
	{
		if (player == nullptr)
		{
			continue;
		}
		player->Update();
	}
	m_currentMap->Update();
	m_currentRiftMap->Update();

	// Camera updates
}

void Game::Render_PlayingMode() const
{
	m_currentMap->Render();
	m_currentRiftMap->Render();
}

void Game::ChangeGameState(GameState newGameState)
{
	m_nextGameState = newGameState;
}

Player* Game::JoinPlayer(int controllerIndex)
{
	Player* newPlayer = new Player(nullptr, Vec3(2.5f, 8.5f, 0.5f));
	int currentIndex = 0;
	bool wasPlayerAdded = false;

	for (int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex)
	{
		if (m_players[playerIndex] == nullptr)
		{
			m_players[playerIndex] = newPlayer;
			currentIndex = playerIndex;
			wasPlayerAdded = true;
			break;
		}
	}
	if (!wasPlayerAdded)
	{
		m_players.push_back(newPlayer);
		currentIndex = (int)m_players.size() - 1;
	}

	m_players[currentIndex]->m_worldCamera = new Camera();
	m_players[currentIndex]->m_screenCamera = new Camera();
	m_players[currentIndex]->m_playerIndex = currentIndex;
	m_players[currentIndex]->m_controllerIndex = controllerIndex;

	return m_players[currentIndex];
}

void Game::SpawnRift(Vec3 position, EulerAngles orientation)
{
	Rift* newRift = new Rift(m_currentMap, m_currentRiftMap, position, orientation, 2.f, 2.f);
	AddRift(newRift);
}

void Game::AddRift(Rift* rift)
{
	for (int riftIndex = 0; riftIndex < s_rifts.size(); ++riftIndex)
	{
		if (s_rifts[riftIndex] == nullptr)
		{
			s_rifts[riftIndex] = rift;
			return;
		}
	}
	s_rifts.push_back(rift);
}

void Game::RemoveRift(Rift* rift)
{
	for (int riftIndex = 0; riftIndex < s_rifts.size(); ++riftIndex)
	{
		if (s_rifts[riftIndex] == rift)
		{
			delete rift;
			s_rifts[riftIndex] = nullptr;
		}
	}
}

void Game::EnterState(GameState state)
{
	switch (state)
	{
		case GameState::GAME_STATE_PLAYING:
		{
			m_currentMap = new Map(this, MapDefinition::GetByName(m_mapDefinitionString));
			m_currentMap->Startup_InitializePlayers();
			m_currentMap->Startup_InitializeActors(); // Initialize actors here because the function calls things that only exist after the map is made.

			m_currentRiftMap = new Map(this, MapDefinition::GetByName(m_riftMapDefinitionString));
			m_currentRiftMap->Startup_InitializeActors();

			SpawnRift(Vec3(16.5f, 15.5f, 2.5f), EulerAngles());

			m_currentMap->m_riftMap = m_currentRiftMap;
			m_currentRiftMap->m_riftMap = m_currentMap;

			bool isMultiplayer = false;
			int playerSize = 0;
			for (Player* player : m_players)
			{
				if (player != nullptr)
				{
					++playerSize;
				}
			}
			if (playerSize > 1)
			{
				isMultiplayer = true;
			}
			for (int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex)
			{
				Player* player = m_players[playerIndex];
				if (player == nullptr)
				{
					continue;
				}
				player->SetViewport(isMultiplayer, playerIndex);
				if (player->m_controllerIndex == -1)
				{
					player->SetControllerState(ControlState::KEYBOARD);
				}
				else if (player->m_controllerIndex == 0)
				{
					player->SetControllerState(ControlState::CONTROLLER);
				}
			}
			g_engine->m_audio->SetNumListeners(playerSize);
			m_gameMusicPlayback = g_engine->m_audio->StartSound(m_gameMusic, true, m_musicVolume);
			g_engine->m_audio->StopSound(m_mainMenuMusicPlayback);
			break;
		}
		case GameState::GAME_STATE_ATTRACT:
		{
			g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
			g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			m_mainMenuMusicPlayback = g_engine->m_audio->StartSound(m_mainMenuMusic, true, m_musicVolume);
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
			for (int riftIndex = 0; riftIndex < s_rifts.size(); ++riftIndex)
			{
				delete s_rifts[riftIndex];
				s_rifts[riftIndex] = nullptr;
			}

			delete m_currentMap;
			m_currentMap = nullptr;

			delete m_currentRiftMap;
			m_currentRiftMap = nullptr;


			g_engine->m_audio->StopSound(m_gameMusicPlayback);
			break;
		}
	}
}