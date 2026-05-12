#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
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
		|| m_game->m_currentGameState == GameState::GAME_STATE_ATTRACT)
	{
		g_engine->m_input->SetCursorMode(CursorMode::POINTER);
	}
	else
	{
		g_engine->m_input->SetCursorMode(CursorMode::FPS);
	}

	m_game->Update();
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
	config.m_windowConfig.m_windowTitle = "Doomenstein";

	// Set up the engine config to be read from EngineConfig.xml, potentially get rid of EngineBuildPreferences?

	Startup_PopulateBlackboard();

	new Engine(config);

	DebugRenderSystemStartup(config.m_debugRenderConfig);
	DebugRenderSetVisible();

	Startup_LoadAllglTF();
	m_game = new Game();

	m_lastFrameTime = (float)GetCurrentTimeSeconds();

	SubscribeEventCallbackFunction("quit", EventQuit);

	std::string testString;

	FileReadToString(testString, "Data/TestFile.bin");


	Startup_DisplayCommandsToDevConsole();
}

void App::Startup_PopulateBlackboard()
{
	XmlDocument gameConfig;
	gameConfig.LoadFile("Data/GameConfig.xml");
	XmlElement* gameConfigRootElement = gameConfig.RootElement();
	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*gameConfigRootElement);
}

void App::Startup_DisplayCommandsToDevConsole()
{
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "CONTROLS: KEYBOARD - CONTROLLER");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Yaw/Pitch Camera: Mouse X/Y input - Right Joystick.");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Move: WASD - Left Joystick");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Sprint: LShift - Left Thumbstick");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Jump: Spacebar - Gamepad A");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Fire: Left Click - Right Trigger");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Look down scope: Right click - Left Trigger");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Reload: R - Gamepad X");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Cycle Weapon: Left/Right Arrow Keys - Left/Right Shoulder");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Select Weapon: 1/2/3/4/5");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Interact with object: E - Gamepad Y");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Toggle Freefly: F");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Pause: P");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Single Step Frame: O");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "Reset/Exit Game: ESC / Back");
	g_engine->m_devConsole->AddLine(DevConsole::DEV_INFO_MAJOR, "----------------");
}

void App::Startup_LoadAllglTF()
{
	XmlDocument gameConfig;
	gameConfig.LoadFile("Data/glTFModels/gltfModels.xml");
	XmlElement* gltfModelsRootElement = gameConfig.RootElement();
	XmlElement* glTFModelElement = gltfModelsRootElement->FirstChildElement("Model");
	while (glTFModelElement)
	{
		std::string gltfPath = ParseXmlAttribute(*glTFModelElement, "gltfModel", "");
		std::string binPath = ParseXmlAttribute(*glTFModelElement, "gltfBin", "");
		glTF_Asset* newAsset = new glTF_Asset(gltfPath.c_str(), binPath.c_str());
		newAsset->m_name = ParseXmlAttribute(*glTFModelElement, "name", "");

		m_gltfModels.push_back(newAsset);
		glTFModelElement = glTFModelElement->NextSiblingElement();
	}
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

void App::GameReset()
{
	// Delete Game
	delete m_game;
	m_game = nullptr;

	// Reinstate Game
	m_game = new Game();
}