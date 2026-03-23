#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/objReader.hpp"
#include "Game/glTFReader.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/XmlUtils.hpp"
#include "Engine/NamedStrings.hpp"
#include "Engine/EngineCommon.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/Texture.hpp"

#include "ThirdParty/stb/stb_image.h"

App* g_app = nullptr;

App::App()
{
	Startup();
}


App::~App()
{
	DebugRenderSystemShutdown();

	delete g_engine;
	delete m_game;

	g_engine = nullptr;
	m_game = nullptr;

	delete m_blaineModel;
	delete m_venipedeModel;
	delete m_6sharksModel;
	delete m_theWindModel;

	m_blaineModel = nullptr;
	m_venipedeModel = nullptr;
	m_6sharksModel = nullptr;
	m_theWindModel = nullptr;
}

void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::RunFrame()
{
	// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
	g_engine->BeginFrame(); // Allow engine subsystems to do pre-frame stuff
	DebugRenderBeginFrame();
	Update();		
	Render();
	DebugRenderEndFrame();
	g_engine->EndFrame(); // Allow engine subsystems to do post-frame stuff
} 

void App::Update()
{
	if (s_systemClock != nullptr) s_systemClock->TickSystemClock();
	// apply the if (wasKeyJustPRessed("O")) {m_ispaused = false; m_ispausedafternextupdate = true;} stuff here for everything.

	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F1))
	{
		m_isDebug = !m_isDebug;
	}

	// Runs update then pauses
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_TILDE))
	{
		g_engine->m_devConsole->ToggleMode(DevConsoleMode::OPEN_FULL);
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	if (g_engine->m_devConsole->GetMode() == DevConsoleMode::OPEN_FULL
		|| m_game->m_currentGameState == GAME_STATE_ATTRACT)
	{
		g_engine->m_input->SetCursorMode(CursorMode::POINTER);
	}
	else
	{
		g_engine->m_input->SetCursorMode(CursorMode::FPS);
	}

	if (m_game->m_isHitStop)
	{
		m_game->m_hitStopTimer -= (float) s_systemClock->GetDeltaSeconds();
		if (m_game->m_hitStopTimer <= 0.f)
		{
			m_game->m_isHitStop = false;
		}
	}
	else
	{
		m_game->Update();
	}
}

void App::Render() const
{
	g_engine->Render();
	m_game->Render();

	// Dev Console
	g_engine->m_devConsole->Render(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void App::Startup()
{
	g_app = this;

	EngineConfig config;
	config.m_windowConfig.m_clientAspect = 2.f;
	config.m_windowConfig.m_windowTitle = "Protogame 3D";

	// Set up the engine config to be read from EngineConfig.xml, potentially get rid of EngineBuildPreferences?

	Startup_PopulateBlackboard();

	new Engine(config);

	DebugRenderSystemStartup(config.m_debugRenderConfig);

	Startup_LoadAllModels();
	Startup_LoadAllglTF();

	m_game = new Game();

	m_lastFrameTime = (float)GetCurrentTimeSeconds();

	SubscribeEventCallbackFunction("quit", EventQuit);

	std::string testString;

	FileReadToString(testString, "Data/TestFile.bin");

	Startup_DisplayCommandsToDevConsole();

	m_textureToDrawTo = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/Blaine.png");
}

void App::Startup_PopulateBlackboard()
{
	XmlDocument gameConfig;
	gameConfig.LoadFile("Data/GameConfig.xml");
	XmlElement* gameConfigRootElement = gameConfig.RootElement();
	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*gameConfigRootElement);
}

void App::Startup_LoadAllModels()
{
	//m_blaineModel = new OBJ_Model("Data/OBJModels/Blaine/blaine.obj", "Data/OBJModels/Blaine/blaine.mtl");
	m_venipedeModel = new OBJ_Model("Data/OBJModels/Venipede/Venipede.obj", "Data/OBJModels/Venipede/Venipede.mtl");
	m_6sharksModel = new OBJ_Model("Data/OBJModels/6sharks/6sharks.obj", "Data/OBJModels/6sharks/6sharks.mtl");
	m_theWindModel = new OBJ_Model("Data/OBJModels/Butler/butler.obj", "Data/OBJModels/Butler/butler.mtl");
}

void App::Startup_LoadAllglTF()
{
	m_dragonModel = new glTF_Asset("Data/glTFModels/Dragon/scene.gltf", "Data/glTFModels/Dragon/scene.bin");
}

void App::Startup_DisplayCommandsToDevConsole()
{
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "CONTROLS:");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Yaw/Pitch Camera: Mouse X/Y input");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Roll Camera: Q/E");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Move Left/Right: A/D");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Move Forward/Back: W/S");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Move Down/Up: Z/C");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Reset Position and Orientation: H");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Increase speed by 10: Hold LShift");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Pause the game: P");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Single step frame: O");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Slow mode: T");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "----------------");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "DEBUG:");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Spawn X-RAY Line: 1");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Spawn Sphere on XY: 2");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Spawn Wireframe Sphere: 3");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Spawn Player Model Basis: 4");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Spawn Full Opposing 3D Text: 5");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Spawn Wireframe Cylinder: 6");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Add Screen Message of Orientation: 7");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Move Portal 1: 8");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Move Portal 2: 9");
}

void App::SetIsQuitting()
{
	m_isQuitting = true;
}

bool App::EventQuit([[maybe_unused]] EventArgs& args)
{
	g_app->SetIsQuitting();
	return false;
}

bool App::IsQuitting() const
{
	if (m_isQuitting)
	{
		return true;
	}
	return false;
}

bool App::IsDebug() const
{
	if (m_isDebug)
	{
		return true;
	}
	return false;
}

Texture* App::getTextureToDrawTo()
{
	return m_textureToDrawTo;
}

void App::GameReset()
{
	// Delete Game
	delete m_game;
	m_game = nullptr;

	// Reinstate Game
	m_game = new Game();
}