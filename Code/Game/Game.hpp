#pragma once
#include "Game/GameCommon.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Renderer/Camera.hpp"

#include <vector>
#include <string>

class RandomNumberGenerator;
class Entity;
class Clock;
class Player;
class Map;
class Timer;
class Shader;
class BitmapFont;
typedef size_t SoundID;

enum class GameState {
	GAME_STATE_NONE,
	GAME_STATE_ATTRACT,
	GAME_STATE_LOBBY,
	GAME_STATE_PLAYING,
	COUNT
};

class Game
{
public:
	Game();
	~Game();
	void Update();
	void Render() const;
	void Startup();

	void AddScreenShake(float screenShake);

	void AddHitStop(float hitStop);
	void ChangeGameState(GameState newGameState);

private:
	void Startup_PopulateFromBlackboard();

	void Update_AttractMode();
	void Update_LobbyMode();
	void Update_PlayingMode();

	void Render_AttractMode() const;
	void Render_LobbyMode() const;
	void Render_PlayingMode() const;

	void EnterState(GameState state);
	void ExitState(GameState state);

public:
	RandomNumberGenerator*	m_randomNumberGenerator = nullptr;
	GameState				m_nextGameState = GameState::GAME_STATE_ATTRACT;
	GameState				m_currentGameState = GameState::GAME_STATE_ATTRACT;
	float					m_screenShakeFraction = 0.f;

	Map* m_currentMap = nullptr;
	Player* m_player = nullptr;
	BitmapFont* m_squirrelFont;

	float m_hitStopTimer = 0.f;
	bool m_isHitStop = false;

	Rgba8 m_backgroundClearColor = Rgba8(50, 50, 50, 255);

	Clock* m_gameClock;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Blackboard values
	float		m_perspectiveFOV = 0.f;
	float		m_rollSensitivity = 0.f;
	float		m_mouseSensitivity = 0.f;
	float		m_controllerSensitivity = 0.f;
	float		m_moveSpeed = 0.f;
	float		m_musicVolume = 0.f;
	SoundID		m_mainMenuMusic;
	SoundID		m_gameMusic;
	SoundID		m_buttonClickSound;
	std::string m_mapDefinitionString;

	Camera* m_screenCamera = nullptr;

	Shader* m_useTexture1Shader;
};