#pragma once
#include "Game/GameCommon.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Renderer/Camera.hpp"

#include <vector>

class RandomNumberGenerator;
class Entity;
class Clock;
class Player;
class Map;
class Timer;
class Shader;
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

private:
	void Startup_PopulateFromBlackboard();

	void Update_AttractMode();
	void Update_LobbyMode();
	void Update_PlayingMode();

	void Render_AttractMode() const;
	void Render_LobbyMode() const;
	void Render_PlayingMode() const;

	bool Update_MainMode_KeyboardInput();
	bool Update_MainMode_ControllerInput();

	void ChangeGameState(GameState newGameState);

public:
	RandomNumberGenerator*	m_randomNumberGenerator = nullptr;
	GameState				m_nextGameState = GameState::GAME_STATE_ATTRACT;
	GameState				m_currentGameState = GameState::GAME_STATE_ATTRACT;
	float					m_screenShakeFraction = 0.f;

	Map* m_currentMap;

	float m_hitStopTimer = 0.f;
	bool m_isHitStop = false;

	Player* m_player;
	Vec3* m_playerTranslationThisFrame;

	Rgba8 m_backgroundClearColor = Rgba8(50, 50, 50, 255);

	Clock* m_gameClock;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Blackboard values
	float m_perspectiveFOV = 0.f;
	float m_rollSensitivity = 0.f;
	float m_mouseSensitivity = 0.f;
	float m_controllerSensitivity = 0.f;
	float m_moveSpeed = 0.f;
	float m_colorUndulateTime = 0.f;
};